# ⚡ IMU Sensor 10DOF & ESP32 BLE Tracker

Hệ thống thu thập và theo dõi dữ liệu cảm biến đa trục (Gia tốc, Con quay, Từ kế) theo thời gian thực sử dụng vi điều khiển **ESP32-S3** kết nối module **10DOF IMU** truyền qua **Web Bluetooth Low Energy (NimBLE)**.

---

## 🛠️ Cấu hình Phần Cứng

- **Vi điều khiển**: ESP32-S3 (4MB Flash, USB Native CDC / JTAG).
- **Giao tiếp I2C**: `SDA = GPIO3`, `SCL = GPIO4` (Tốc độ 400kHz).
- **Cảm biến tích hợp trên module 10DOF**:
  - **Gia tốc kế 3 trục (Accelerometer)**: ADXL345 (`0x53`) — Dải đo $\pm 2.0g$.
  - **Con quay hồi chuyển 3 trục (Gyroscope)**: ITG-3200 / MPU-6050 (`0x68`) — Dải đo $\pm 2000^\circ/\text{s}$, tự động cân chỉnh độ lệch tĩnh (Zero-offset Auto Calibration).
  - **Từ kế 3 trục (Magnetometer / Compass)**: QMC5883L (`0x0C`) — Dải đo $\pm 8\text{ Gauss}$, cập nhật 200Hz.
  - **Khí áp kế / Độ cao**: BMP280 (`0x77`).

---

## 📱 Giao Diện Web & Tính Năng Thu Thập Dataset (`index.html`)

- **Đồ thị thời gian thực**: Trực quan hóa 3 kênh dữ liệu (Gia tốc, Con quay, Từ kế) với độ trễ siêu thấp (tần số 50Hz).
- **La bàn số (Digital Compass)**: Tính toán góc định hướng $\theta = \text{atan2}(m_y, m_x)$ và cường độ từ trường $|\vec{M}|$.
- **Nhãn hành vi & Sự cố**:
  - *Bình thường*: Đi bộ (`walking`), Chạy bộ (`running`), Đứng yên (`standing`), Ngồi (`sitting`), Nằm (`lying`), Cầu thang (`stairs`), Nhảy (`jumping`), Đạp xe (`cycling`).
  - *Sự cố*: Ngã (`fall`), Khó thở (`dyspnea`).
- **Xuất dữ liệu CSV đa năng**:
  - Định dạng chuẩn 9 trục + timestamp + activity label:
    ```csv
    timestamp_ms,accX_g,accY_g,accZ_g,gyrX_dps,gyrY_dps,gyrZ_dps,magX_uT,magY_uT,magZ_uT,activity
    ```
  - Hỗ trợ **Web Share API** (chia sẻ nhanh qua Zalo/Drive/Files), **Sao chép 1 chạm** và **Tải trực tiếp `.csv`**.

---

## 🚀 Hướng Dẫn Nạp Firmware & Sử Dụng

### 1. Nạp Firmware (PlatformIO)
```bash
cd firmware_platformio
pio run -e esp32s3 -t upload
```

### 2. Mở Giao diện Web
1. Mở tệp `index.html` bằng trình duyệt Google Chrome hoặc Microsoft Edge.
2. Bấm **"Kết nối Bluetooth"** và chọn thiết bị **`ESP32_IMU`**.
3. Bắt đầu xem dữ liệu và thu thập tập dữ liệu CSV!
