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

// 18 bytes = 9 x int16 Little-Endian
int16_t packet[9] = {0};

float ax = 0.0f, ay = 0.0f, az = 0.0f;
float gx = 0.0f, gy = 0.0f, gz = 0.0f;
float mx = 0.0f, my = 0.0f, mz = 0.0f;

float gx_offset = 0.0f, gy_offset = 0.0f, gz_offset = 0.0f;

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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[SYSTEM] Khoi dong ESP32-S3 9DOF BLE 50Hz...");

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeOut(10);
  initSensors();

  NimBLEDevice::init("ESP32_IMU");
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);

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

  Serial.println("[BLE] Dang phat song Bluetooth 'ESP32_IMU' - SAN SANG!");
}

unsigned long lastTime = 0;
unsigned long lastPrint = 0;

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

    if (now - lastPrint >= 500) {
      lastPrint = now;
      Serial.printf("ACC[%.2f, %.2f, %.2f] | GYR[%.1f, %.1f, %.1f] | MAG[%.1f, %.1f, %.1f] | BLE: %s\n",
                    ax, ay, az, gx, gy, gz, mx, my, mz,
                    deviceConnected ? "DA KET NOI (50Hz)" : "DANG CHO KET NOI");
    }
  }
}
