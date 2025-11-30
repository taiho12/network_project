# Hệ Thống Đặt Vé Máy Bay Online (TCP Socket)

Dự án mô phỏng hệ thống đặt vé máy bay sử dụng mô hình **Client – Server** viết bằng **ngôn ngữ C** và giao tiếp qua **TCP Socket (Winsock2)**.

---

## Chức năng

- Đăng ký tài khoản  
- Đăng nhập  
- Tìm kiếm chuyến bay  
- Đặt vé máy bay  
- Thanh toán bằng thẻ Visa / Napas (mô phỏng)  
- Xem vé đã đặt  
- Hủy vé  
- Sinh mã vé tự động (T001, T002, …)  

---

## Cấu trúc dự án

- client.c
- server.c
- flight.txt
- database.txt
- ticket.txt
- README.md

---

## Hướng dẫn chạy

### 1. Biên dịch server
gcc server.c -o server -lws2_32

### 2. Chạy server
./server

### 3. Biên dịch client
gcc client.c -o client -lws2_32

### 4. Chạy client
./client

---

## Ghi chú

- Dự án chạy trên **Windows** (Winsock2).
- Cơ sở dữ liệu được lưu trong file `.txt`.
- Hệ thống thanh toán chỉ mang tính mô phỏng, không thật.

---

## Tác giả
- Tên:  Nguyễn Anh Thứ 
        Nguyễn Hồ Tấn Tài
- Môn học: Lập Trình Mạng
