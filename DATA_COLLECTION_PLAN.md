# Kế hoạch & Hướng dẫn Chi tiết Thu thập Dataset IMU 9DOF (10 Người - 10 Hoạt động)

---

## 🎯 1. Quy mô & Thông số Dataset
- **Số lượng người tham gia**: **10 người** (`sub01` $\rightarrow$ `sub10`) (Đa dạng nam/nữ, chiều cao, cân nặng).
- **Quy cách thu thập**: **Mỗi hoạt động thực hiện 5 lần (trials), mỗi lần kéo dài đúng 1 phút (60 giây)**.
- **Tần số lấy mẫu**: **50 Hz** (50 dòng/giây = **3.000 dòng/phút / file**).
- **Tổng số file CSV mục tiêu**: $10\text{ hoạt động} \times 5\text{ lần} \times 10\text{ người} = \mathbf{500\text{ files CSV}}$.
- **Tổng dung lượng Dataset**: $500\text{ files} \times 3.000\text{ dòng} = \mathbf{1.500.000\text{ dòng dữ liệu 9DOF chuẩn}}$.

---

## 📋 2. BẢNG HƯỚNG DẪN CHI TIẾT CÁCH ĐO TỪNG HOẠT ĐỘNG

> **Vị trí & Chiều gắn cảm biến cố định cho TẤT CẢ các bài đo**:
> - Gắn cảm biến ở **chính giữa vùng Bụng (ngang thắt lưng / trên rốn)** bằng đai co giãn hoặc băng dán.
> - Cổng cắm cáp USB luôn hướng **LÊN TRÊN**, mặt chip cảm biến hướng **RA NGOÀI**.

| STT | Tên Hoạt động | Label Key | Số lần $\times$ Thời gian | Tư thế & Thao tác thực hiện chi tiết | Dấu hiệu tín hiệu IMU kỳ vọng |
| :---: | :--- | :---: | :---: | :--- | :--- |
| **1** | **Đi bộ** | `walking` | **5 lần $\times$ 1 phút** | Đi bộ tự nhiên tốc độ bình thường ($3 \sim 4.5\text{ km/h}$) trên mặt phẳng (hành lang, phòng rộng hoặc sân). Đi thẳng, quay đầu nhẹ nhàng khi hết đường. | Sóng gia tốc $A_Y, A_Z$ tuần hoàn đều đặn, biên độ êm ($0.8g \sim 1.3g$), tần số bước $\approx 1.8\text{ Hz}$. |
| **2** | **Chạy bộ** | `running` | **5 lần $\times$ 1 phút** | Chạy bước nhỏ, tốc độ vừa phải (chạy dưỡng sinh / jogging $6 \sim 8\text{ km/h}$). Nhịp thở và guồng chân đều đặn. | Sóng gia tốc dao động mạnh hơn đi bộ ($1.5g \sim 2.5g$), tần số bước $\approx 2.5 \sim 3.0\text{ Hz}$. |
| **3** | **Chạy gấp rút** | `sprinting` | **5 lần $\times$ 1 phút** | Chạy bứt tốc nhanh, guồng chân mạnh mẽ và dậm gót dứt khoát (như khi đang vội chạy đuổi theo xe bus hoặc chạy nước rút). | Biên độ gia tốc va đập rất lớn ($>2.5g \sim 4.0g$), $G_X, G_Y$ dao động mạnh do thân người lắc theo sải chạy. |
| **4** | **Đứng yên** | `standing` | **5 lần $\times$ 1 phút** | Đứng thẳng tự nhiên trên 2 chân. Thỉnh thoảng có thể dồn trọng tâm đổi chân trụ hoặc cử động tay nhẹ như khi đứng chờ đợi thực tế. | $A_Y \approx 1.0g$ (trọng lực), $A_X \approx 0.0g, A_Z \approx 0.0g$. Con quay $G_X, G_Y, G_Z \approx 0^\circ/\text{s}$. |
| **5** | **Ngồi** | `sitting` | **5 lần $\times$ 1 phút** | Ngồi ghế tựa hoặc ghế làm việc. Ngồi thả lỏng tự nhiên, có thể hơi ngả lưng vào ghế hoặc cầm điện thoại bấm nhẹ nhàng. | $A_Y \approx 0.9 \sim 1.0g$ (tùy độ ngả ghế), đồ thị cực kỳ phẳng và tĩnh, không có rung lắc bước chân. |
| **6** | **Nằm** | `lying` | **5 lần $\times$ 1 phút** | Nằm trên giường hoặc nệm phẳng. Mỗi file 1 phút có thể nằm ngửa (30s) rồi đổi tư thế nằm nghiêng hoặc nằm sấp (30s). | Khi nằm ngửa: $A_Z \approx +1.0g, A_Y \approx 0.0g$. Khi nằm sấp: $A_Z \approx -1.0g$. Khi nằm nghiêng: $A_X \approx \pm 1.0g$. |
| **7** | **Cầu thang** | `stairs` | **5 lần $\times$ 1 phút** | Đi lên và đi xuống cầu thang bộ nhịp nhàng. Đi từng bậc một tốc độ tự nhiên. | Sóng gia tốc có độ dốc lệch pha khi nhấc chân bước lên bậc hoặc tiếp đất khi bước xuống bậc. |
| **8** | **Nhảy** | `jumping` | **5 lần $\times$ 1 phút** | Nhảy bật 2 chân tại chỗ hoặc nhảy dây nhẹ nhàng. Cứ nhảy $5 \sim 10$ nhịp rồi nghỉ 2 giây rồi nhảy tiếp trong suốt 1 phút. | Đồ thị xuất hiện các đỉnh gai gia tốc nhọn đối xứng ($>2.5g$ khi tiếp đất và $\approx 0.0g$ lúc đang ở trên không). |
| **9** | **Thở nhanh (Sự cố)** | `tachypnea` | **5 lần $\times$ 1 phút** | Ngồi hoặc nằm yên bất động, **thở dốc dồn dập $25 \sim 35$ nhịp/phút** (mô phỏng như vừa chạy mệt kiệt sức hoặc lên cơn hen/suy hô hấp). | Thân người tĩnh ($M_X, M_Y, M_Z$ đứng yên), nhưng trục $A_Z$ và $G_X$ xuất hiện **sóng nhọn dồn dập tần số cao** theo nhịp thở co thắt bụng. |
| **10** | **Ngã (Sự cố)** | `fall` | **5 lần $\times$ 1 phút** | **BẮT BUỘC THỰC HIỆN TRÊN NỆM DÀY / XỐP BẢO HỘ**:<br>Trong 1 phút, thực hiện $2 \sim 3$ cú ngã khác nhau (Đứng 10s $\rightarrow$ Ngã xuống nệm nằm im 10s $\rightarrow$ Đứng dậy ngã kiểu khác).<br>• *Lần 1*: Ngã sấp về trước.<br>• *Lần 2*: Ngã ngửa ra sau.<br>• *Lần 3*: Ngã nghiêng sang trái/phải.<br>• *Lần 4*: Trượt chân ngã khi đang đi.<br>• *Lần 5*: Ngã tự do bất ngờ. | Có **1 đỉnh xung gia tốc va đập cực mạnh ($>3.0g \sim 4.5g$)**, con quay $G_X, G_Y$ xoay đột ngột, sau đó vector trọng lực chuyển ngay sang trạng thái Nằm ($A_Z \approx 1.0g$). |

---

## 📁 3. Quy ước Đặt tên File CSV Đồng bộ

Tất cả các file tải về từ web app đổi tên theo cấu trúc:
```text
sub[Mã_Người]_[Tên_Hoạt_Động]_t0[Lần_Thử].csv
```

### Ví dụ danh sách 50 file của Người 01 (`sub01`):
```text
sub01_walking_t01.csv    -> sub01_walking_t05.csv
sub01_running_t01.csv    -> sub01_running_t05.csv
sub01_sprinting_t01.csv  -> sub01_sprinting_t05.csv
sub01_standing_t01.csv   -> sub01_standing_t05.csv
sub01_sitting_t01.csv    -> sub01_sitting_t05.csv
sub01_lying_t01.csv      -> sub01_lying_t05.csv
sub01_stairs_t01.csv     -> sub01_stairs_t05.csv
sub01_jumping_t01.csv    -> sub01_jumping_t05.csv
sub01_tachypnea_t01.csv  -> sub01_tachypnea_t05.csv
sub01_fall_t01.csv       -> sub01_fall_t05.csv
```
*(Tương tự cho `sub02` đến `sub10`)*.

---

## 🚀 4. Quy trình Thao tác Thu Thập Từng File trên Web App
1. **Bước 1**: Mở Web App [https://hoa-10.github.io/IMU_sensor_10DOF/](https://hoa-10.github.io/IMU_sensor_10DOF/) $\rightarrow$ Bấm **"Kết nối Bluetooth"** $\rightarrow$ Chọn **`ESP32_IMU`**.
2. **Bước 2**: Chọn nút **Nhãn hoạt động** tương ứng (ví dụ: `Đi bộ`).
3. **Bước 3**: Người tham gia chuẩn bị sẵn sàng $\rightarrow$ Bấm **"▶ Bắt đầu ghi"**.
4. **Bước 4**: Quan sát đồng hồ trên web, khi đếm đến đúng **01:00 (1 phút)** $\rightarrow$ Bấm **"⏹ Dừng ghi"**.
5. **Bước 5**: Bấm **"💾 Tải / Xuất CSV"** $\rightarrow$ Bấm **"📥 Tải file .csv về máy"** $\rightarrow$ Lưu vào thư mục tương ứng.
