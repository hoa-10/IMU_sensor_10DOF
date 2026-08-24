# Kế hoạch & Quy chuẩn Thu thập Dataset IMU 9DOF (10 Người - 10 Hoạt động Chuẩn)

---

## 🎯 1. Cấu trúc 10 Hoạt động & Sự cố (10 Thư mục / 10 Classes)

Dataset gồm đúng **10 nhóm (10 classes)** rõ ràng, phục vụ bài toán nhận diện hành vi (HAR) và phát hiện sự cố y tế:

### A. Nhóm Hoạt động Hàng ngày (8 Classes):
1. **Đi bộ** (`walking`)
2. **Chạy bộ** (`running`) — Chạy đều, chạy dưỡng sinh (jogging $6 \sim 8\text{ km/h}$).
3. **Chạy gấp rút** (`sprinting`) — Chạy bứt tốc nhanh, guồng chân mạnh.
4. **Đứng yên** (`standing`)
5. **Ngồi** (`sitting`)
6. **Nằm** (`lying`)
7. **Cầu thang** (`stairs`)
8. **Nhảy** (`jumping`)

### B. Nhóm Sự cố / Khẩn cấp (2 Classes):
9. **Ngã** (`fall`) — Các pha ngã sấp, ngã ngửa, ngã nghiêng, trượt chân trên nệm bảo hộ.
10. **Thở nhanh** (`tachypnea`) — Thở dồn dập, thở dốc $25 \sim 35$ nhịp/phút (như mệt hụt hơi, lên cơn hen suyễn/suy hô hấp).

---

## 📋 2. Bảng Phân bổ Thu thập Cho 1 Người (5 Lần x 1 Phút)

| STT | Tên Hoạt động | Label Key | Số lần (Hiệp) | Thời gian mỗi lần | Số dòng thu được / Người | Hướng dẫn thực hiện thực tế |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| **1** | **Đi bộ** | `walking` | **5 lần** | **1 phút** | 15.000 dòng | Đi bộ tự nhiên tốc độ bình thường ($3 \sim 4.5\text{ km/h}$). |
| **2** | **Chạy bộ** | `running` | **5 lần** | **1 phút** | 15.000 dòng | Chạy bước nhỏ, nhịp đều đặn ($6 \sim 8\text{ km/h}$). |
| **3** | **Chạy gấp rút** | `sprinting` | **5 lần** | **1 phút** | 15.000 dòng | Chạy bứt tốc nhanh, guồng chân mạnh. |
| **4** | **Đứng yên** | `standing` | **5 lần** | **1 phút** | 15.000 dòng | Đứng tự nhiên, thỉnh thoảng đổi chân trụ. |
| **5** | **Ngồi** | `sitting` | **5 lần** | **1 phút** | 15.000 dòng | Ngồi ghế tựa, ngồi làm việc, bấm điện thoại. |
| **6** | **Nằm** | `lying` | **5 lần** | **1 phút** | 15.000 dòng | Nằm ngửa, nằm sấp hoặc nằm nghiêng trên giường/nệm. |
| **7** | **Cầu thang** | `stairs` | **5 lần** | **1 phút** | 15.000 dòng | Đi lên và đi xuống cầu thang bộ nhịp nhàng. |
| **8** | **Nhảy** | `jumping` | **5 lần** | **1 phút** | 15.000 dòng | Nhảy 2 chân tại chỗ hoặc nhảy lò cò. |
| **9** | **Thở nhanh** | `tachypnea` | **5 lần** | **1 phút** | 15.000 dòng | Ngồi hoặc nằm yên, thở dồn dập, thở dốc $25 \sim 35$ nhịp/phút. |
| **10** | **Ngã** | `fall` | **5 lần** | **1 phút** | 15.000 dòng | **NGÃ TRÊN NỆM BẢO HỘ** (mỗi file 1 phút thực hiện $2 \sim 3$ cú ngã: ngã sấp, ngã ngửa, ngã nghiêng, trượt chân). |

---

## 📁 3. Quy ước Đặt tên File & Lưu trữ Thư mục
Lưu file theo định dạng:
```text
sub[XX]_[activity]_t0[Y].csv
```

**Ví dụ thực tế:**
- `sub01_walking_t01.csv` $\rightarrow$ `sub01_walking_t05.csv`
- `sub01_tachypnea_t01.csv` $\rightarrow$ `sub01_tachypnea_t05.csv`
- `sub01_fall_t01.csv` $\rightarrow$ `sub01_fall_t05.csv`
- ... tiếp tục cho đến Người 10 (`sub10_...`).

---

## 📊 4. Tổng quy mô sau khi thu đủ 10 người
- **Tổng số file CSV**: $10\text{ classes} \times 5\text{ lần} \times 10\text{ người} = \mathbf{500\text{ files CSV}}$.
- **Tổng số dòng**: $500 \times 3.000 = \mathbf{1.500.000\text{ dòng dữ liệu 9DOF chuẩn}}$.
