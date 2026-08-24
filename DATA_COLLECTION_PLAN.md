# Kế hoạch & Quy chuẩn Thu thập Dữ liệu Dataset IMU 9DOF (ESP32-S3)

---

## 🎯 1. Mục tiêu & Thông số Kỹ thuật Dataset
- **Mục đích**: Huấn luyện mô hình Học máy / Học sâu (1D-CNN, LSTM, CNN-LSTM, Random Forest, XGBoost) nhận diện 8 hoạt động hàng ngày (ADL) và 3 sự cố y tế khẩn cấp.
- **Tần số lấy mẫu**: **50 Hz** (50 dòng/giây = **3.000 dòng/phút**).
- **Cấu trúc dữ liệu**: 9 trục chuẩn hóa ($a_x, a_y, a_z$ đơn vị $g$; $g_x, g_y, g_z$ đơn vị $^\circ/\text{s}$; $m_x, m_y, m_z$ đơn vị $\mu\text{T}$).
- **Kỹ thuật trích xuất cửa sổ (Windowing)**: Cửa sổ trượt **2.56 giây (128 mẫu)**, độ gối đầu (overlap) **50% (64 mẫu)**.

---

## 👥 2. Đối tượng & Số lượng Dữ liệu Cần thiết

Để mô hình AI đạt độ chính xác cao ($>92\%$) và không bị học vẹt (Overfitting) theo dáng đi của 1 người:
- **Số lượng người tham gia**: Khuyến nghị từ **3 đến 5 người** (nam/nữ, chiều cao/cân nặng khác nhau).
- **Tổng thời lượng kỳ vọng**: Khoảng **1.5 đến 2 giờ dữ liệu thực tế** (tương đương $\approx 250.000 \sim 350.000$ dòng dữ liệu).

---

## 📋 3. Bảng Chi tiết Kế hoạch Thu thập Từng Nhãn

### Nhóm A: Hoạt động Chu kỳ / Trạng thái Liên tục
| Nhãn hoạt động | Tên Label Key | Thời lượng mỗi hiệp | Số hiệp / Người | Tổng mẫu mục tiêu / Người | Hướng dẫn thực hiện thực tế |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Đi bộ** | `walking` | 2 phút | 3 hiệp | ~18.000 dòng | Đi bộ tự nhiên tốc độ bình thường ($3 \sim 4.5\text{ km/h}$) trên hành lang/phòng. |
| **Chạy bộ** | `running` | 1.5 phút | 3 hiệp | ~13.500 dòng | Chạy bước nhỏ, nhịp đều đặn (jogging $6 \sim 8\text{ km/h}$). |
| **Chạy gấp rút** | `sprinting` | 30 giây | 4 hiệp | ~6.000 dòng | Chạy bứt tốc nhanh, dậm chân mạnh. |
| **Đứng yên** | `standing` | 2 phút | 2 hiệp | ~12.000 dòng | Đứng tự nhiên, thỉnh thoảng đổi chân trụ hoặc cử động người nhẹ. |
| **Ngồi** | `sitting` | 2 phút | 2 hiệp | ~12.000 dòng | Ngồi ghế tựa, ngồi làm việc, bấm điện thoại tự nhiên. |
| **Nằm** | `lying` | 2 phút | 2 hiệp | ~12.000 dòng | Nằm ngửa (1 phút), nằm sấp (30s), nằm nghiêng (30s). |
| **Cầu thang** | `stairs` | 1.5 phút | 3 hiệp | ~13.500 dòng | Đi lên và đi xuống cầu thang bộ nhịp nhàng. |

---

### Nhóm B: Sự cố & Hoạt động Xung Tức thời
| Nhãn hoạt động | Tên Label Key | Cách thức thu thập | Số lần thực hiện / Người | Lưu ý an toàn & Kỹ thuật |
| :--- | :---: | :--- | :---: | :--- |
| **Ngã** | `fall` | Ghi từng pha ngã (5 giây/pha) | **15 - 20 lần ngã** | **BẮT BUỘC NGÃ TRÊN NỆM DÀY/XỐP BẢO HỘ**: <br>• Ngã sấp về trước (5 lần)<br>• Ngã ngửa ra sau (5 lần)<br>• Ngã nghiêng sang trái/phải (5 lần)<br>• Trượt chân ngã khi đang đi (5 lần) |
| **Nhảy** | `jumping` | Nhảy liên tục 30 giây | 3 hiệp (~4.500 dòng) | Nhảy tại chỗ 2 chân hoặc nhảy lò cò. |

---

### Nhóm C: Bất thường Hô hấp / Nhịp thở
| Nhãn hoạt động | Tên Label Key | Thời lượng mỗi hiệp | Số hiệp / Người | Cách thức mô phỏng |
| :--- | :---: | :--- | :---: | :--- |
| **Thở nhanh (Thở dốc)** | `tachypnea` | 1 phút | 3 hiệp (~9.000 dòng) | Ngồi/nằm yên, thở dồn dập $25 \sim 35$ nhịp/phút (như vừa chạy mệt hụt hơi). |
| **Thở chậm (Ngưng thở)** | `bradypnea` | 1 phút | 3 hiệp (~9.000 dòng) | Thở rất chậm ($6 \sim 8$ nhịp/phút) hoặc nín thở từng đoạn $10 \sim 15$ giây. |

---

## ⚙️ 4. Quy tắc Chuẩn hóa Vị trí & Thiết bị (Data Consistency Protocol)

1. **Vị trí cố định**: Đeo ở **chính giữa vùng Bụng** (ngay trên thắt lưng/rốn).
2. **Chiều cảm biến cố định**: 
   - Đầu cắm cổng sạc/cáp luôn hướng **LÊN TRÊN**.
   - Mặt có chip cảm biến hướng **RA NGOÀI**.
3. **Quy tắc khi bấm nút thu dữ liệu**:
   - Chuẩn bị tư thế sẵn sàng $\rightarrow$ Bấm **"▶ Bắt đầu ghi"**.
   - Thực hiện liên tục trong khoảng thời gian quy định.
   - Dừng hành động $\rightarrow$ Bấm **"⏹ Dừng ghi"** $\rightarrow$ Bấm **"📥 Tải file .csv về máy"**.

---

## 📁 5. Quy ước Đặt tên File CSV để Huấn luyện AI

Đặt tên file theo cấu trúc chuẩn:
```text
[Người]_[Nhãn]_[Hiệp]_[Ngày].csv
```
*Ví dụ cụ thể:*
- `sub01_walking_trial01.csv`
- `sub01_fall_forward_trial01.csv`
- `sub02_tachypnea_trial02.csv`
- `sub03_sprinting_trial01.csv`

---

## 🗓️ 6. Lộ trình Thực hiện Gợi ý (3 Ngày)

- **Ngày 1 (Hoạt động tĩnh & Đi lại cơ bản)**: Thu thập `standing`, `sitting`, `lying`, `walking`, `tachypnea`, `bradypnea` cho 3 người.
- **Ngày 2 (Hoạt động cường độ cao)**: Thu thập `running`, `sprinting`, `stairs`, `jumping`.
- **Ngày 3 (Thử nghiệm Ngã an toàn & Ghép Dataset)**: Chuẩn bị nệm bảo hộ, thu thập nhãn `fall` cho từng người, tổng hợp và kiểm tra độ cân bằng dữ liệu (Class Balance).
