#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>
#include <arduinoFFT.h>
#include <math.h>

#define SDA_PIN 3
#define SCL_PIN 4

#define ADXL345_ADDR 0x53
#define GYRO_ADDR    0x68
#define QMC_MAG_ADDR 0x0C

// Pulse Sensor (analog, PPG quang hoc) - chan ADC1 con trong, khong dung chung voi I2C
#define PULSE_PIN 5

#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"
// Characteristic dieu khien Start/Stop + nhan ket qua nhip tim SOTA
#define SOTA_CHARACTERISTIC_UUID "19b10003-e8f2-537e-4f6c-d104768a1214"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
NimBLECharacteristic* pSotaCharacteristic = nullptr;
bool deviceConnected = false;

// 12 x int16 Little-Endian = 24 bytes (goi tin stream lien tuc 50Hz)
// [0-2] Accel g*1000 | [3-5] Gyro dps*10 | [6-8] Mag uT*10
// [9] Pulse raw ADC (0-4095) | [10] Quick BPM (chi de xem live, KHONG phai so chinh thuc)
// [11] IBI ms (khoang cach nhip gan nhat, tinh nhanh on-device)
int16_t packet[12] = {0};

float ax = 0.0f, ay = 0.0f, az = 0.0f;
float gx = 0.0f, gy = 0.0f, gz = 0.0f;
float mx = 0.0f, my = 0.0f, mz = 0.0f;

float gx_offset = 0.0f, gy_offset = 0.0f, gz_offset = 0.0f;

// ===================== PULSE SENSOR (Quick Live Estimate) =====================
// Thuat toan bat dinh nguong dong (adaptive threshold), gion, chi de hien thi
// realtime cho nguoi dung biet dang do duoc nhip hay khong trong luc dang ghi.
int pulse_filter_buf[4] = {2000, 2000, 2000, 2000};
int pulse_filter_idx = 0;

float pulse_dc_baseline = 2000.0f;
float pulse_ac_signal = 0.0f;
float pulse_peak_amp = 15.0f;
float pulse_dyn_thresh = 5.0f;

volatile int pulseQuickBPM = 0;
volatile int pulseIBI = 750;
unsigned long pulseLastBeatTime = 0;
bool pulseDetected = false;
int pulse_rate_history[8] = {750, 750, 750, 750, 750, 750, 750, 750};
int pulse_rate_idx = 0;
int pulseRawLatest = 0;

// ===================== SOTA HEART RATE (chay tren ESP32 khi bam Dung) =========
// Thuat toan: Bandpass Butterworth bac 4 (zero-phase 2 chieu) + Elgendi Two-
// Moving-Average peak detection + FFT pho tan so + hop nhat da mien (giong het
// logic RobustHeartRateEstimator trong advanced_pulse_dsp.py, port sang C++).
#define MAX_SOTA_SAMPLES 3500   // ~70s @ 50Hz, du cho 1 trial 60s
#define FFT_SIZE 4096
#define SOTA_MIN_SAMPLES 250    // toi thieu ~5s du lieu moi xu ly

// He so Butterworth bac 4, dai thong 0.6-3.5Hz, thiet ke san cho fs = 50Hz
// (tinh bang scipy.signal.butter(4, [0.6/25, 3.5/25], btype='band'))
const double SOTA_B[9] = {
  0.0007137440532057703, 0.0, -0.002854976212823081, 0.0, 0.004282464319234622,
  0.0, -0.002854976212823081, 0.0, 0.0007137440532057703
};
const double SOTA_A[9] = {
  1.0, -6.932259722227774, 21.155700947148283, -37.135715384773256,
  41.020079732012086, -29.202040616383258, 13.085233668814215,
  -3.3744497221913585, 0.38345187201731606
};
const int SOTA_FILT_ORDER = 8; // do dai b,a - 1

int16_t pulseBuf[MAX_SOTA_SAMPLES];
volatile int pulseBufCount = 0;
volatile bool pulseRecording = false;
volatile bool sotaProcessRequested = false;
unsigned long pulseRecStartMillis = 0;
unsigned long pulseRecEndMillis = 0;

ArduinoFFT<float> FFT = ArduinoFFT<float>();

// Loc IIR Direct Form II Transposed (1 chieu / causal)
void iirFilterDF2T(const float* in, float* out, int n, const double* b, const double* a, int order) {
  double z[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (int i = 0; i < n; i++) {
    double x = in[i];
    double y = b[0] * x + z[0];
    for (int j = 0; j < order - 1; j++) {
      z[j] = b[j + 1] * x + z[j + 1] - a[j + 1] * y;
    }
    z[order - 1] = b[order] * x - a[order] * y;
    out[i] = (float)y;
  }
}

// Loc 2 chieu xap xi filtfilt (khong dem bien, chap nhan duoc vi da cat bo
// doan dau 1.5s truoc khi bat dinh)
void filtfiltApprox(const float* in, float* out, int n) {
  float* tmp = new float[n];
  iirFilterDF2T(in, tmp, n, SOTA_B, SOTA_A, SOTA_FILT_ORDER);
  for (int i = 0; i < n / 2; i++) { float t = tmp[i]; tmp[i] = tmp[n - 1 - i]; tmp[n - 1 - i] = t; }
  iirFilterDF2T(tmp, out, n, SOTA_B, SOTA_A, SOTA_FILT_ORDER);
  for (int i = 0; i < n / 2; i++) { float t = out[i]; out[i] = out[n - 1 - i]; out[n - 1 - i] = t; }
  delete[] tmp;
}

// Trung binh dong kieu numpy.convolve(mode='same'), zero-pad ngoai bien
void movingAvgSame(const float* x, int n, int w, float* out) {
  int half = w / 2;
  for (int i = 0; i < n; i++) {
    float sum = 0;
    for (int k = 0; k < w; k++) {
      int idx = i - half + k;
      if (idx >= 0 && idx < n) sum += x[idx];
    }
    out[i] = sum / (float)w;
  }
}

void insertionSortFloat(float* arr, int n) {
  for (int i = 1; i < n; i++) {
    float key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) { arr[j + 1] = arr[j]; j--; }
    arr[j + 1] = key;
  }
}

// Percentile noi suy tuyen tinh giong numpy.percentile mac dinh (arr da sap xep)
float percentileSorted(const float* sortedArr, int n, float p) {
  if (n == 1) return sortedArr[0];
  float idx = (p / 100.0f) * (n - 1);
  int lo = (int)floorf(idx);
  int hi = (int)ceilf(idx);
  if (lo == hi) return sortedArr[lo];
  float frac = idx - lo;
  return sortedArr[lo] + (sortedArr[hi] - sortedArr[lo]) * frac;
}

float medianOf(float* arr, int n) {
  float* cp = new float[n];
  memcpy(cp, arr, n * sizeof(float));
  insertionSortFloat(cp, n);
  float m = (n % 2 == 1) ? cp[n / 2] : (cp[n / 2 - 1] + cp[n / 2]) / 2.0f;
  delete[] cp;
  return m;
}

// Bat dinh Elgendi Two-Moving-Average tren tin hieu da chuan hoa z-score
// (z, do dai n). Ghi cac chi so dinh vao peaksOut, tra ve so luong dinh.
int elgendiPeakDetection(const float* z, int n, float fs, int* peaksOut, int maxPeaks) {
  int trimSamples = (int)(1.5f * fs);
  int offset = 0;
  const float* signalEval = z;
  int m = n;
  if (n > trimSamples * 2) {
    signalEval = z + trimSamples;
    m = n - trimSamples;
    offset = trimSamples;
  }

  float* squared = new float[m];
  float meanSquared = 0;
  for (int i = 0; i < m; i++) {
    float v = signalEval[i];
    if (v < 0) v = 0;
    squared[i] = v * v;
    meanSquared += squared[i];
  }
  meanSquared /= (float)m;

  int nPeak = (int)((120.0f / 1000.0f) * fs); if (nPeak < 3) nPeak = 3;
  int nBeat = (int)((650.0f / 1000.0f) * fs); if (nBeat < 7) nBeat = 7;

  float* maPeak = new float[m];
  float* maBeat = new float[m];
  movingAvgSame(squared, m, nPeak, maPeak);
  movingAvgSame(squared, m, nBeat, maBeat);

  int minSpacing = (int)(0.36f * fs);
  int peakCount = 0;
  bool inBlock = false;
  int blockStart = 0;

  for (int i = 0; i < m; i++) {
    float threshold = maBeat[i] + 0.15f * meanSquared;
    bool boi = maPeak[i] > threshold;
    if (boi && !inBlock) {
      inBlock = true;
      blockStart = i;
    } else if (!boi && inBlock) {
      inBlock = false;
      int blockEnd = i;
      int localPeak = blockStart;
      float best = signalEval[blockStart];
      for (int k = blockStart; k < blockEnd; k++) {
        if (signalEval[k] > best) { best = signalEval[k]; localPeak = k; }
      }
      int actualIdx = localPeak + offset;
      if (peakCount == 0 || (actualIdx - peaksOut[peakCount - 1]) >= minSpacing) {
        if (peakCount < maxPeaks) peaksOut[peakCount++] = actualIdx;
      }
    }
  }

  delete[] squared;
  delete[] maPeak;
  delete[] maBeat;
  return peakCount;
}

struct SotaResult {
  float fusedBpm;
  float timeBpm;
  float spectralBpm;
  float ibiMedianMs;
  float rmssdMs;
  int confidence; // -1 = khong du du lieu, 0 = low, 1 = medium, 2 = high
  int peakCount;
  float fsUsed;
};

SotaResult computeSotaHeartRate(const int16_t* rawBuf, int n, float fs) {
  SotaResult r = {0, 0, 0, 0, 0, -1, 0, fs};
  if (n < SOTA_MIN_SAMPLES) return r;

  // 1. Detrend (tru trung binh)
  float mean = 0;
  for (int i = 0; i < n; i++) mean += rawBuf[i];
  mean /= (float)n;

  float* detr = new float[n];
  for (int i = 0; i < n; i++) detr[i] = (float)rawBuf[i] - mean;

  // 2. Loc bandpass zero-phase xap xi (thay filtfilt)
  float* filtered = new float[n];
  filtfiltApprox(detr, filtered, n);
  delete[] detr;

  // 3. Chuan hoa Z-score
  float fmean = 0;
  for (int i = 0; i < n; i++) fmean += filtered[i];
  fmean /= (float)n;
  float fvar = 0;
  for (int i = 0; i < n; i++) fvar += (filtered[i] - fmean) * (filtered[i] - fmean);
  float fstd = sqrtf(fvar / (float)n) + 1e-6f;

  float* z = new float[n];
  for (int i = 0; i < n; i++) z[i] = (filtered[i] - fmean) / fstd;

  // 4. Bat dinh Elgendi
  int maxPeaks = 300;
  int* peaks = new int[maxPeaks];
  int peakCount = elgendiPeakDetection(z, n, fs, peaks, maxPeaks);
  delete[] z;
  r.peakCount = peakCount;

  // 5. Uoc tinh pho tan so (FFT thay Welch) tren tin hieu da loc (chua z-norm)
  static float vReal[FFT_SIZE];
  static float vImag[FFT_SIZE];
  int nWin = (n < FFT_SIZE) ? n : FFT_SIZE;
  for (int i = 0; i < nWin; i++) {
    float w = 0.5f - 0.5f * cosf(2.0f * PI * i / (float)(nWin - 1));
    vReal[i] = filtered[i] * w;
    vImag[i] = 0.0f;
  }
  for (int i = nWin; i < FFT_SIZE; i++) { vReal[i] = 0.0f; vImag[i] = 0.0f; }
  delete[] filtered;

  FFT.compute(vReal, vImag, FFT_SIZE, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, FFT_SIZE);

  float freqRes = fs / (float)FFT_SIZE;
  int kMin = (int)ceilf(0.7f / freqRes);
  int kMax = (int)floorf(3.0f / freqRes);
  if (kMax >= FFT_SIZE / 2) kMax = FFT_SIZE / 2 - 1;

  int peakK = kMin;
  float peakMag = vReal[kMin];
  float sumMag = 0;
  int cnt = 0;
  float* bandMags = new float[kMax - kMin + 1];
  for (int k = kMin; k <= kMax; k++) {
    bandMags[k - kMin] = vReal[k];
    if (vReal[k] > peakMag) { peakMag = vReal[k]; peakK = k; }
    sumMag += vReal[k];
    cnt++;
  }
  insertionSortFloat(bandMags, cnt);
  float medianMag = bandMags[cnt / 2];
  delete[] bandMags;

  float dominantFreq = peakK * freqRes;
  float spectralBpm = dominantFreq * 60.0f;
  float peakPower = peakMag * peakMag;
  float noisePower = medianMag * medianMag;
  float snrRatio = peakPower / (noisePower + 1e-6f);

  r.spectralBpm = spectralBpm;

  // 6. Chi so mien thoi gian tu cac dinh + loc ngoai lai IQR + hop nhat
  if (peakCount < 3) {
    r.fusedBpm = spectralBpm;
    r.timeBpm = 0;
    r.confidence = 0; // low
    delete[] peaks;
    return r;
  }

  int nIbi = peakCount - 1;
  float* rawIbis = new float[nIbi];
  for (int i = 0; i < nIbi; i++) {
    rawIbis[i] = (peaks[i + 1] - peaks[i]) * (1000.0f / fs);
  }
  delete[] peaks;

  float* sortedIbis = new float[nIbi];
  memcpy(sortedIbis, rawIbis, nIbi * sizeof(float));
  insertionSortFloat(sortedIbis, nIbi);
  float q25 = percentileSorted(sortedIbis, nIbi, 25.0f);
  float q75 = percentileSorted(sortedIbis, nIbi, 75.0f);
  delete[] sortedIbis;
  float iqr = q75 - q25;
  float lowerBound = fmaxf(350.0f, q25 - 1.5f * iqr);
  float upperBound = fminf(1500.0f, q75 + 1.5f * iqr);

  float* cleanIbis = new float[nIbi];
  int nClean = 0;
  for (int i = 0; i < nIbi; i++) {
    if (rawIbis[i] >= lowerBound && rawIbis[i] <= upperBound) {
      cleanIbis[nClean++] = rawIbis[i];
    }
  }
  if (nClean < 2) {
    memcpy(cleanIbis, rawIbis, nIbi * sizeof(float));
    nClean = nIbi;
  }
  delete[] rawIbis;

  float* timeBpms = new float[nClean];
  for (int i = 0; i < nClean; i++) timeBpms[i] = 60000.0f / cleanIbis[i];
  float timeBpm = medianOf(timeBpms, nClean);
  delete[] timeBpms;
  float ibiMedian = medianOf(cleanIbis, nClean);

  float rmssd = 0;
  if (nClean > 1) {
    float sumSq = 0;
    for (int i = 0; i < nClean - 1; i++) {
      float d = cleanIbis[i + 1] - cleanIbis[i];
      sumSq += d * d;
    }
    rmssd = sqrtf(sumSq / (float)(nClean - 1));
  }
  delete[] cleanIbis;

  float bpmDiff = fabsf(timeBpm - spectralBpm);
  float fusedBpm;
  int confidence;
  if (bpmDiff <= 4.0f && snrRatio > 3.0f) {
    confidence = 2; // high
    fusedBpm = 0.6f * timeBpm + 0.4f * spectralBpm;
  } else if (bpmDiff <= 8.0f) {
    confidence = 1; // medium
    fusedBpm = 0.5f * timeBpm + 0.5f * spectralBpm;
  } else {
    confidence = 0; // low
    fusedBpm = spectralBpm;
  }

  r.fusedBpm = fusedBpm;
  r.timeBpm = timeBpm;
  r.ibiMedianMs = ibiMedian;
  r.rmssdMs = rmssd;
  r.confidence = confidence;
  return r;
}

// Ket qua SOTA gui qua BLE: 8 x int16 = 16 byte
// [0] fused_bpm*10 [1] time_bpm*10 [2] spectral_bpm*10 [3] ibi_median_ms
// [4] rmssd_ms [5] confidence (-1/0/1/2) [6] peak_count [7] fs_used*10
int16_t sotaResultPacket[8] = {0};

void runSotaAnalysis() {
  int n = pulseBufCount;
  unsigned long elapsedMs = pulseRecEndMillis - pulseRecStartMillis;
  float fs = (n > 1 && elapsedMs > 0) ? ((float)n / (elapsedMs / 1000.0f)) : 50.0f;

  Serial.printf("[SOTA] Bat dau phan tich: %d mau, fs uoc tinh = %.2f Hz\n", n, fs);
  unsigned long t0 = millis();

  SotaResult res = computeSotaHeartRate(pulseBuf, n, fs);

  unsigned long t1 = millis();
  Serial.printf("[SOTA] Xong trong %lu ms | Fused: %.1f BPM | Time: %.1f | Spectral: %.1f | IBI: %.0fms | RMSSD: %.0fms | Peaks: %d | Do tin cay: %d\n",
                t1 - t0, res.fusedBpm, res.timeBpm, res.spectralBpm, res.ibiMedianMs, res.rmssdMs, res.peakCount, res.confidence);

  sotaResultPacket[0] = (int16_t)(res.fusedBpm * 10.0f);
  sotaResultPacket[1] = (int16_t)(res.timeBpm * 10.0f);
  sotaResultPacket[2] = (int16_t)(res.spectralBpm * 10.0f);
  sotaResultPacket[3] = (int16_t)(res.ibiMedianMs);
  sotaResultPacket[4] = (int16_t)(res.rmssdMs);
  sotaResultPacket[5] = (int16_t)(res.confidence);
  sotaResultPacket[6] = (int16_t)(res.peakCount);
  sotaResultPacket[7] = (int16_t)(res.fsUsed * 10.0f);

  if (pSotaCharacteristic != nullptr) {
    pSotaCharacteristic->setValue((uint8_t*)sotaResultPacket, sizeof(sotaResultPacket));
    if (deviceConnected) pSotaCharacteristic->notify();
  }
}

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("[BLE] Client Connected!");
    };
    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("[BLE] Client Disconnected, restarting advertising...");
        NimBLEDevice::startAdvertising();
    }
};

class SotaControlCallbacks: public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) {
    std::string val = pChar->getValue();
    if (val.length() < 1) return;
    uint8_t cmd = (uint8_t)val[0];
    if (cmd == 1) {
      pulseBufCount = 0;
      pulseRecStartMillis = millis();
      pulseRecEndMillis = pulseRecStartMillis;
      pulseRecording = true;
      Serial.println("[SOTA] Bat dau dem PPG cho phan tich SOTA...");
    } else if (cmd == 0) {
      pulseRecording = false;
      sotaProcessRequested = true;
      Serial.println("[SOTA] Dung dem, cho xu ly SOTA trong loop()...");
    }
  }
};

void writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
}

void initSensors() {
  Serial.println("[INIT] Khoi tao module 10DOF tren GPIO 3 (SDA) & GPIO 4 (SCL)...");

  // 1. ADXL345 (Gia toc)
  Wire.beginTransmission(ADXL345_ADDR);
  if (Wire.endTransmission(true) == 0) {
    writeReg(ADXL345_ADDR, 0x2D, 0x08);
    writeReg(ADXL345_ADDR, 0x31, 0x08);
    Serial.println("  [+] ADXL345 OK (0x53)");
  }

  // 2. Gyroscope ITG3200
  Wire.beginTransmission(GYRO_ADDR);
  if (Wire.endTransmission(true) == 0) {
    writeReg(GYRO_ADDR, 0x6B, 0x00);
    writeReg(GYRO_ADDR, 0x3E, 0x00);
    writeReg(GYRO_ADDR, 0x16, 0x18);
    writeReg(GYRO_ADDR, 0x6A, 0x00);
    writeReg(GYRO_ADDR, 0x37, 0x02);
    Serial.println("  [+] ITG3200 OK (0x68)");
  }

  // 3. QMC5883L (Tu ke)
  Wire.beginTransmission(QMC_MAG_ADDR);
  if (Wire.endTransmission(true) == 0) {
    writeReg(QMC_MAG_ADDR, 0x0B, 0x01);
    writeReg(QMC_MAG_ADDR, 0x09, 0x1D);
    Serial.println("  [+] QMC5883L OK (0x0C)");
  }

  // 4. Pulse Sensor (analog PPG)
  Serial.println("[INIT] Khoi tao Pulse Sensor tren GPIO 5 (ADC)...");
  analogReadResolution(12);
  analogSetPinAttenuation(PULSE_PIN, ADC_11db);

  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(PULSE_PIN);
    delay(5);
  }
  pulse_dc_baseline = (float)sum / 50.0f;
  for (int i = 0; i < 4; i++) pulse_filter_buf[i] = (int)pulse_dc_baseline;
  pulseLastBeatTime = millis();
  Serial.println("  [+] Pulse Sensor OK (GPIO 5)");
}

void readSensors() {
  // 1. Gia toc ADXL345
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(0x32);
  if (Wire.endTransmission(true) == 0) {
    if (Wire.requestFrom((int)ADXL345_ADDR, 6) == 6) {
      int16_t rx = Wire.read() | (Wire.read() << 8);
      int16_t ry = Wire.read() | (Wire.read() << 8);
      int16_t rz = Wire.read() | (Wire.read() << 8);
      ax = (float)rx * 0.0039f;
      ay = (float)ry * 0.0039f;
      az = (float)rz * 0.0039f;
    }
  }

  // 2. Con quay Gyro ITG3200
  Wire.beginTransmission(GYRO_ADDR);
  Wire.write(0x1D);
  if (Wire.endTransmission(true) == 0) {
    if (Wire.requestFrom((int)GYRO_ADDR, 6) == 6) {
      int16_t rx = (Wire.read() << 8) | Wire.read();
      int16_t ry = (Wire.read() << 8) | Wire.read();
      int16_t rz = (Wire.read() << 8) | Wire.read();
      gx = ((float)rx / 14.375f) - gx_offset;
      gy = ((float)ry / 14.375f) - gy_offset;
      gz = ((float)rz / 14.375f) - gz_offset;
    }
  }

  // 3. Tu ke QMC5883L
  Wire.beginTransmission(QMC_MAG_ADDR);
  Wire.write(0x0A);
  Wire.write(0x01);
  Wire.endTransmission(true);

  Wire.beginTransmission(QMC_MAG_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(true) == 0) {
    if (Wire.requestFrom((int)QMC_MAG_ADDR, 6) == 6) {
      int16_t rx = Wire.read() | (Wire.read() << 8);
      int16_t ry = Wire.read() | (Wire.read() << 8);
      int16_t rz = Wire.read() | (Wire.read() << 8);
      mx = (float)rx / 30.0f;
      my = (float)ry / 30.0f;
      mz = (float)rz / 30.0f;
    }
  }
}

// Doc + xu ly nhanh 1 mau Pulse Sensor: loc trung binh dong, tach baseline DC,
// bat dinh bang nguong dong (adaptive threshold) -> chi phuc vu hien thi live.
void readPulseQuick() {
  int raw = analogRead(PULSE_PIN);
  pulseRawLatest = raw;

  // 1. Loc trung binh dong 4 mau
  pulse_filter_buf[pulse_filter_idx] = raw;
  pulse_filter_idx = (pulse_filter_idx + 1) % 4;
  float smoothed = (pulse_filter_buf[0] + pulse_filter_buf[1] + pulse_filter_buf[2] + pulse_filter_buf[3]) / 4.0f;

  // 2. Tach baseline DC troi cham
  pulse_dc_baseline += 0.015f * (smoothed - pulse_dc_baseline);

  // 3. Tin hieu song xung AC
  pulse_ac_signal = smoothed - pulse_dc_baseline;

  unsigned long now = millis();
  unsigned long timeSinceLast = now - pulseLastBeatTime;

  // 4. Cap nhat nguong dong do nhay cao (Min threshold = 4.0)
  if (pulse_ac_signal > pulse_dyn_thresh) {
    if (pulse_ac_signal > pulse_peak_amp) {
      pulse_peak_amp = pulse_ac_signal;
    }
  } else {
    pulse_peak_amp *= 0.997f; // Suy hao nhe
    if (pulse_peak_amp < 8.0f) pulse_peak_amp = 8.0f;
  }
  pulse_dyn_thresh = pulse_peak_amp * 0.45f;
  if (pulse_dyn_thresh < 3.5f) pulse_dyn_thresh = 3.5f;

  // 5. Bat dinh nhip tim (300ms - 1500ms ~ 40-200 BPM)
  if (timeSinceLast > 320) {
    if ((pulse_ac_signal > pulse_dyn_thresh) && !pulseDetected && (timeSinceLast > (unsigned long)(pulseIBI * 0.55f))) {
      pulseDetected = true;
      pulseIBI = timeSinceLast;
      pulseLastBeatTime = now;

      pulse_rate_history[pulse_rate_idx] = pulseIBI;
      pulse_rate_idx = (pulse_rate_idx + 1) % 8;

      long sumIBI = 0;
      for (int i = 0; i < 8; i++) sumIBI += pulse_rate_history[i];
      int avgIBI = sumIBI / 8;

      if (avgIBI > 0) {
        int calcBPM = 60000 / avgIBI;
        if (calcBPM >= 45 && calcBPM <= 180) {
          pulseQuickBPM = calcBPM;
        }
      }
    }
  }

  if (pulse_ac_signal < (pulse_dyn_thresh * 0.2f) && pulseDetected) {
    pulseDetected = false;
  }

  if (timeSinceLast > 2500) {
    pulseQuickBPM = 0;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[SYSTEM] Khoi dong ESP32-S3 9DOF + Pulse Sensor SOTA BLE 50Hz...");

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeOut(10);
  initSensors();

  // Tang MTU de goi tin 24 byte (12 x int16) di lot trong 1 notify, khong bi cat
  NimBLEDevice::init("ESP32_IMU");
  NimBLEDevice::setMTU(185);
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  pCharacteristic->setValue((uint8_t*)packet, sizeof(packet));

  pSotaCharacteristic = pService->createCharacteristic(
      SOTA_CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE
  );
  pSotaCharacteristic->setCallbacks(new SotaControlCallbacks());
  pSotaCharacteristic->setValue((uint8_t*)sotaResultPacket, sizeof(sotaResultPacket));

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setName("ESP32_IMU");
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("[BLE] Dang phat song Bluetooth 'ESP32_IMU' - SAN SANG!");
}

unsigned long lastTime = 0;
unsigned long lastPrint = 0;

void loop() {
  unsigned long now = millis();
  if (now - lastTime >= 20) { // 50 Hz - dung nhip voi dataset IMU dang thu
    lastTime = now;

    readSensors();
    readPulseQuick();

    if (pulseRecording && pulseBufCount < MAX_SOTA_SAMPLES) {
      pulseBuf[pulseBufCount++] = (int16_t)pulseRawLatest;
      pulseRecEndMillis = now;
    }

    // 9 x int16 IMU (18 bytes)
    packet[0] = (int16_t)(ax * 1000.0f);
    packet[1] = (int16_t)(ay * 1000.0f);
    packet[2] = (int16_t)(az * 1000.0f);
    packet[3] = (int16_t)(gx * 10.0f);
    packet[4] = (int16_t)(gy * 10.0f);
    packet[5] = (int16_t)(gz * 10.0f);
    packet[6] = (int16_t)(mx * 10.0f);
    packet[7] = (int16_t)(my * 10.0f);
    packet[8] = (int16_t)(mz * 10.0f);

    // 3 x int16 Pulse (them 6 byte -> 24 byte)
    packet[9]  = (int16_t)pulseRawLatest;
    packet[10] = (int16_t)pulseQuickBPM;
    packet[11] = (int16_t)pulseIBI;

    if (deviceConnected && pCharacteristic != nullptr) {
      pCharacteristic->setValue((uint8_t*)packet, sizeof(packet));
      pCharacteristic->notify();
    }

    if (now - lastPrint >= 500) {
      lastPrint = now;
      Serial.printf("ACC[%.2f, %.2f, %.2f] | GYR[%.1f, %.1f, %.1f] | MAG[%.1f, %.1f, %.1f] | PULSE raw:%d quickBPM:%d IBI:%d | Buf:%d | BLE: %s\n",
                    ax, ay, az, gx, gy, gz, mx, my, mz,
                    pulseRawLatest, pulseQuickBPM, pulseIBI, pulseBufCount,
                    deviceConnected ? "DA KET NOI (50Hz)" : "DANG CHO KET NOI");
    }
  }

  // Xu ly SOTA duoc yeu cau tu app (chay 1 lan, ngoai nhip 50Hz de khong lam
  // treo viec doc cam bien/BLE lau hon can thiet)
  if (sotaProcessRequested) {
    sotaProcessRequested = false;
    runSotaAnalysis();
  }
}
