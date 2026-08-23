#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>

#define SDA_PIN 3
#define SCL_PIN 4

#define ADXL345_ADDR 0x53
#define GYRO_ADDR    0x68
#define QMC_MAG_ADDR 0x0C

#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CHARACTERISTIC_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

// 18 bytes = 9 x int16
int16_t packet[9] = {0};

float ax = 0.0f, ay = 0.0f, az = 0.0f;
float gx = 0.0f, gy = 0.0f, gz = 0.0f;
float mx = 0.0f, my = 0.0f, mz = 0.0f;

bool isITG3200 = false;
bool isMPU6050 = false;
float gx_offset = 0.0f, gy_offset = 0.0f, gz_offset = 0.0f;

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
    };
    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        NimBLEDevice::startAdvertising();
    }
};

uint8_t readReg(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)addr, (size_t)1, (bool)true);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
}

void initSensors() {
  // 1. ADXL345
  writeReg(ADXL345_ADDR, 0x2D, 0x08);
  writeReg(ADXL345_ADDR, 0x31, 0x08);

  // 2. Gyroscope
  uint8_t who75 = readReg(GYRO_ADDR, 0x75);
  if (who75 == 0x68 || who75 == 0x71 || who75 == 0x70) {
    isMPU6050 = true;
    writeReg(GYRO_ADDR, 0x6B, 0x00);
  } else {
    isITG3200 = true;
    writeReg(GYRO_ADDR, 0x3E, 0x00);
    writeReg(GYRO_ADDR, 0x16, 0x18);
  }

  // 3. QMC5883L Magnetometer
  writeReg(QMC_MAG_ADDR, 0x0A, 0x80); // Reset
  delay(50);
  writeReg(QMC_MAG_ADDR, 0x0B, 0x01); // Set period
  delay(10);
  writeReg(QMC_MAG_ADDR, 0x09, 0x1D); // Continuous 200Hz, 8G, 512 OSR
  delay(20);

  // Calib gyro tĩnh
  float sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 50; i++) {
    uint8_t reg = isMPU6050 ? 0x43 : 0x1D;
    Wire.beginTransmission(GYRO_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)GYRO_ADDR, (size_t)6, (bool)true);
    if (Wire.available() >= 6) {
      int16_t rx = (Wire.read() << 8) | Wire.read();
      int16_t ry = (Wire.read() << 8) | Wire.read();
      int16_t rz = (Wire.read() << 8) | Wire.read();
      float scale = isMPU6050 ? 131.0f : 14.375f;
      sx += (float)rx / scale;
      sy += (float)ry / scale;
      sz += (float)rz / scale;
    }
    delay(20);
  }
  gx_offset = sx / 50.0f;
  gy_offset = sy / 50.0f;
  gz_offset = sz / 50.0f;
}

void readSensors() {
  // 1. Gia tốc ADXL345
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(0x32);
  Wire.endTransmission(false);
  if (Wire.requestFrom((uint8_t)ADXL345_ADDR, (size_t)6, (bool)true) >= 6) {
    int16_t rx = Wire.read() | (Wire.read() << 8);
    int16_t ry = Wire.read() | (Wire.read() << 8);
    int16_t rz = Wire.read() | (Wire.read() << 8);
    ax = (float)rx * 0.0039f;
    ay = (float)ry * 0.0039f;
    az = (float)rz * 0.0039f;
  }

  // 2. Con quay Gyro
  uint8_t reg = isMPU6050 ? 0x43 : 0x1D;
  Wire.beginTransmission(GYRO_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  if (Wire.requestFrom((uint8_t)GYRO_ADDR, (size_t)6, (bool)true) >= 6) {
    int16_t rx = (Wire.read() << 8) | Wire.read();
    int16_t ry = (Wire.read() << 8) | Wire.read();
    int16_t rz = (Wire.read() << 8) | Wire.read();
    float scale = isMPU6050 ? 131.0f : 14.375f;
    gx = ((float)rx / scale) - gx_offset;
    gy = ((float)ry / scale) - gy_offset;
    gz = ((float)rz / scale) - gz_offset;
  }

  // 3. Từ kế QMC5883L
  Wire.beginTransmission(QMC_MAG_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  if (Wire.requestFrom((uint8_t)QMC_MAG_ADDR, (size_t)6, (bool)true) >= 6) {
    int16_t rx = Wire.read() | (Wire.read() << 8);
    int16_t ry = Wire.read() | (Wire.read() << 8);
    int16_t rz = Wire.read() | (Wire.read() << 8);
    mx = (float)rx / 30.0f; // uT
    my = (float)ry / 30.0f;
    mz = (float)rz / 30.0f;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  initSensors();

  NimBLEDevice::init("ESP32_IMU");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  pCharacteristic->setValue((uint8_t*)packet, sizeof(packet));
  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setName("ESP32_IMU");
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
}

unsigned long lastTime = 0;

void loop() {
  unsigned long now = millis();
  if (now - lastTime >= 20) { // 50 Hz
    lastTime = now;

    readSensors();

    // 9 x int16 (18 bytes)
    packet[0] = (int16_t)(ax * 1000.0f);
    packet[1] = (int16_t)(ay * 1000.0f);
    packet[2] = (int16_t)(az * 1000.0f);
    packet[3] = (int16_t)(gx * 10.0f);
    packet[4] = (int16_t)(gy * 10.0f);
    packet[5] = (int16_t)(gz * 10.0f);
    packet[6] = (int16_t)(mx * 10.0f);
    packet[7] = (int16_t)(my * 10.0f);
    packet[8] = (int16_t)(mz * 10.0f);

    if (deviceConnected && pCharacteristic != nullptr) {
      pCharacteristic->setValue((uint8_t*)packet, sizeof(packet));
      pCharacteristic->notify();
    }
  }
}
