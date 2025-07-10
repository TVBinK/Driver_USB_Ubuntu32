# 🚀 HƯỚNG DẪN NHANH - Driver USB Caesar Cipher

## ⚡ Cách chạy nhanh nhất

```bash
# 1. Biên dịch driver
make

# 2. Nạp module driver
sudo insmod usb_crypto_vi.ko

lsusb | grep "0951:1666"
ls /sys/bus/usb/devices/ | grep "4-"
#3. Unbind khỏi usb-storage và bind lại vào usb_crypto
echo "4-1:1.0" | sudo tee /sys/bus/usb/drivers/usb-storage/unbind 
echo "4-1:1.0" | sudo tee /sys/bus/usb/drivers/usb_crypto/bind

# 5. Kiểm tra
ls /sys/bus/usb/drivers/usb_crypto/
ls -la /dev/crypto0
# 6. Cấp quyền truy cập thiết bị
sudo chmod 666 /dev/crypto0
```

## 🧪 Test nhanh

```bash
# Mã hóa
echo "E:Hello World" > /dev/crypto0
cat /dev/crypto0

# Giải mã
echo "D:Uryyb Jbeyq" > /dev/crypto0
cat /dev/crypto0

#tu dong chay
./demo.sh

```
```bash
#Cách Kernel kết nối với USB
- Khởi tạo Driver (Module Init)
# Khi cắm USB Kingston, kernel tự động:
# 1. Đọc Device Descriptor từ USB
# 2. Lấy idVendor và idProduct ví dụ: VID:PID = 0x0951:0x1666
# 3. Tìm driver phù hợp trong registered drivers xem thiết bị có khớp hay ko
# 4. Gọi hàm ham_ket_noi của driver tìm thấy
# 5. Khởi tạo device structure (crypto_dev)
# 6. Tạo character device (/dev/crypto0)
# 7. Thiết bị sẵn sàng nhận lệnh mã hóa/giải mã
```