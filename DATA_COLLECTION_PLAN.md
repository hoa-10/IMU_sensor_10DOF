# Kế hoạch & Quy chuẩn Thu thập Dataset IMU 9DOF (Chuẩn 10 Người - 10 Subjects)

---

## 🎯 1. Tổng quan Quy mô Dataset
- **Số lượng người tham gia**: **10 người** (`sub01` $\rightarrow$ `sub10`) — Nam/Nữ, chiều cao/cân nặng đa dạng.
- **Thời gian cho 1 người**: Khoảng **12 $\sim$ 15 phút / người**.
- **Tổng thời lượng dữ liệu 10 người**: $\approx \mathbf{2.5 \sim 3\text{ giờ}}$ dữ liệu thực tế ($\approx \mathbf{450.000 \sim 500.000\text{ dòng CSV}}$).
- **Tần số lấy mẫu**: **50 Hz** (50 mẫu/giây = **3.000 dòng/phút**).

---

## 📋 2. Bảng Phân bổ Chi tiết Cho 1 Người (Per Subject Protocol)

| STT | Hoạt động / Sự cố | Label Key | Số lần (Hiệp) | Thời gian mỗi lần | Tổng thời gian / 1 Người | Số dòng thu được / 1 Người | Hướng dẫn thực hiện thực tế |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **1** | **Đi bộ** | `walking` | **2 lần** | 1.5 phút (90s) | 3 phút | ~9.000 dòng | Đi bộ tự nhiên tốc độ bình thường ($3 \sim 4.5\text{ km/h}$). |
| **2** | **Chạy bộ** | `running` | **2 lần** | 1 phút (60s) | 2 phút | ~6.000 dòng | Chạy bước nhỏ, nhịp đều đặn (jogging $6 \sim 8\text{ km/h}$). |
| **3** | **Chạy gấp rút** | `sprinting` | **2 lần** | 30 giây | 1 phút | ~3.000 dòng | Chạy bứt tốc nhanh, dậm chân mạnh. |
| **4** | **Đứng yên** | `standing` | **1 lần** | 2 phút (120s) | 2 phút | ~6.000 dòng | Đứng tự nhiên, thỉnh thoảng đổi chân trụ hoặc cử động nhẹ. |
| **5** | **Ngồi** | `sitting` | **1 lần** | 2 phút (120s) | 2 phút | ~6.000 dòng | Ngồi ghế tựa, ngồi làm việc, bấm điện thoại tự nhiên. |
| **6** | **Nằm** | `lying` | **1 lần** | 2 phút (120s) | 2 phút | ~6.000 dòng | 1p nằm ngửa, 30s nằm sấp, 30s nằm nghiêng. |
| **7** | **Cầu thang** | `stairs` | **2 lần** | 1 phút (60s) | 2 phút | ~6.000 dòng | 1 lần đi lên, 1 lần đi xuống cầu thang bộ. |
| **8** | **Nhảy** | `jumping` | **2 lần** | 20 giây | 40 giây | ~2.000 dòng | Nhảy 2 chân tại chỗ hoặc nhảy lò cò. |
| **9** | **Thở nhanh** | `tachypnea` | **2 lần** | 45 giây | 1.5 phút | ~4.500 dòng | Ngồi/nằm yên, thở dồn dập $25 \sim 35$ nhịp/phút (như mệt đứt hơi). |
| **10** | **Thở chậm** | `bradypnea` | **2 lần** | 45 giây | 1.5 phút | ~4.500 dòng | Thở rất chậm ($6 \sim 8$ nhịp/phút) hoặc nín thở $10 \sim 15$ giây. |
| **11** | **Ngã (Sự cố)** | `fall` | **8 - 10 lần** | 5 giây / lần | ~1 phút | ~2.500 dòng | **NGÃ TRÊN NỆM BẢO HỘ**:<br>• 2 lần ngã sấp về trước<br>• 2 lần ngã ngửa ra sau<br>• 2 lần ngã nghiêng trái/phải<br>• 2 lần trượt chân ngã khi đang đi |

---

## 📁 3. Quy ước Đặt tên File CSV Cho 10 Người

Để khi nạp vào Python huấn luyện không bị nhầm lẫn, hãy lưu file theo cấu trúc:
```text
sub[XX]_[activity]_[trial].csv
```

**Ví dụ cụ thể:**
- Người số 1:
  - `sub01_walking_t01.csv`, `sub01_walking_t02.csv`
  - `sub01_running_t01.csv`, `sub01_running_t02.csv`
  - `sub01_fall_forward_t01.csv`, `sub01_fall_backward_t01.csv`
  - `sub01_tachypnea_t01.csv`, `sub01_bradypnea_t01.csv`
- Người số 2:
  - `sub02_walking_t01.csv`, `sub02_walking_t02.csv`, ...
- ... tiếp tục đến `sub10`.

---

## ⚙️ 4. Checklist Trước Khi Thu Thập Cho Mỗi Người
1. [ ] Cố định cảm biến chặt ở **vùng Bụng (ngang rốn/thắt lưng)**.
2. [ ] Đầu cắm cáp hướng **LÊN TRÊN**, mặt chip cảm biến hướng **RA NGOÀI**.
3. [ ] Kết nối Bluetooth $\rightarrow$ Chọn nhãn $\rightarrow$ Bấm **"▶ Bắt đầu ghi"**.
4. [ ] Hết thời gian $\rightarrow$ Bấm **"⏹ Dừng ghi"** $\rightarrow$ Bấm **"📥 Tải file .csv về máy"** $\rightarrow$ Đổi tên file đúng quy ước.
