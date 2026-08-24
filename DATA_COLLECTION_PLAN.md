# Kế hoạch & Quy chuẩn Thu thập Dataset IMU 9DOF (Chuẩn 10 Người - 5 Lần x 1 Phút)

---

## 🎯 1. Tổng quan Quy mô Dataset
- **Số lượng người tham gia**: **10 người** (`sub01` $\rightarrow$ `sub10`) — Nam/Nữ, chiều cao/cân nặng đa dạng.
- **Quy cách thu thập**: **Mỗi hoạt động thực hiện 5 lần (trials), mỗi lần kéo dài đúng 1 phút (60 giây)**.
- **Tần số lấy mẫu**: **50 Hz** (50 mẫu/giây = **3.000 dòng/phút / file**).
- **Tổng số file CSV mục tiêu**: $11\text{ hoạt động} \times 5\text{ lần} \times 10\text{ người} = \mathbf{550\text{ files CSV}}$.
- **Tổng dung lượng Dataset**: $550\text{ files} \times 3.000\text{ dòng} = \mathbf{1.650.000\text{ dòng dữ liệu 9DOF chuẩn}}$.

---

## 📋 2. Bảng Phân bổ Chi tiết Cho 1 Người (10 Người làm y hệt nhau)

| STT | Hoạt động / Sự cố | Label Key | Số lần thu / Người | Thời gian mỗi lần | Số dòng / 1 Người (5 file) | Hướng dẫn thực hiện thực tế |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| **1** | **Đi bộ** | `walking` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Đi bộ tự nhiên tốc độ bình thường ($3 \sim 4.5\text{ km/h}$). |
| **2** | **Chạy bộ** | `running` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Chạy bước nhỏ, nhịp đều đặn (jogging $6 \sim 8\text{ km/h}$). |
| **3** | **Chạy gấp rút** | `sprinting` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Chạy bứt tốc nhanh, dậm chân mạnh. |
| **4** | **Đứng yên** | `standing` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Đứng tự nhiên, thỉnh thoảng đổi chân trụ hoặc cử động nhẹ. |
| **5** | **Ngồi** | `sitting` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Ngồi ghế tựa, ngồi làm việc, bấm điện thoại tự nhiên. |
| **6** | **Nằm** | `lying` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Nằm ngửa, nằm sấp hoặc nằm nghiêng tự nhiên trên giường/nệm. |
| **7** | **Cầu thang** | `stairs` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Đi lên và đi xuống cầu thang bộ nhịp nhàng. |
| **8** | **Nhảy** | `jumping` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Nhảy 2 chân tại chỗ hoặc nhảy lò cò. |
| **9** | **Thở nhanh** | `tachypnea` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Ngồi/nằm yên, thở dồn dập $25 \sim 35$ nhịp/phút (như mệt hụt hơi). |
| **10** | **Thở chậm** | `bradypnea` | **5 lần** | **1 phút** (60s) | 15.000 dòng | Thở rất chậm ($6 \sim 8$ nhịp/phút) hoặc nín thở từng đoạn $10 \sim 15\text{s}$. |
| **11** | **Ngã (Sự cố)** | `fall` | **5 lần** | **1 phút** (60s) | 15.000 dòng | **NGÃ TRÊN NỆM BẢO HỘ** (mỗi file 1 phút thực hiện $2 \sim 3$ cú ngã: ngã sấp, ngã ngửa, ngã nghiêng, trượt chân). |

---

## 📁 3. Quy ước Đặt tên File CSV Chuẩn hóa

Lưu file theo định dạng:
```text
sub[XX]_[activity]_t0[Y].csv
```

**Ví dụ thực tế:**
- Người 01: `sub01_walking_t01.csv`, `sub01_walking_t02.csv`, ..., `sub01_walking_t05.csv`
- Người 01 ngã: `sub01_fall_t01.csv`, ..., `sub01_fall_t05.csv`
- Người 02: `sub02_walking_t01.csv`, ..., `sub02_walking_t05.csv`
- ... tiếp tục cho đến Người 10 (`sub10_...`).

---

## ⚙️ 4. Quy tắc Chuẩn hóa Thiết bị & Vị trí
1. **Vị trí cố định**: Đeo ở **chính giữa vùng Bụng** (ngay trên thắt lưng/rốn).
2. **Chiều cảm biến cố định**: Đầu cắm cáp hướng **LÊN TRÊN**, mặt chip cảm biến hướng **RA NGOÀI**.
3. **Quy trình ghi mỗi file**:
   - Tư thế sẵn sàng $\rightarrow$ Bấm **"▶ Bắt đầu ghi"**.
   - Thực hiện đúng **1 phút (bấm Dừng khi đồng hồ điểm 01:00)**.
   - Bấm **"📥 Tải file .csv về máy"** $\rightarrow$ Đổi tên file đúng cú pháp.
