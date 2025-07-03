# 🚀 QUICK START - Driver USB Caesar Cipher

## ⚡ Cách chạy nhanh nhất

```bash
# 1. Build driver
make

# Hoặc fix thủ công
USB_DEV="4-1" 
echo "${USB_DEV}:1.0" | sudo tee /sys/bus/usb/drivers/usb-storage/unbind
echo "${USB_DEV}:1.0" | sudo tee /sys/bus/usb/drivers/usb_crypto/bind
sudo chmod 666 /dev/crypto0


# 3. Test ngay
./demo.sh

## 🧪 Test nhanh

```bash
# Mã hóa
echo "E:Hello World" > /dev/crypto0
cat /dev/crypto0

# Giải mã
echo "D:Uryyb Jbeyq" > /dev/crypto0  
cat /dev/crypto0
```

## 📖 Đọc thêm
```Tìm USB device
lsusb | grep "0951:1666"
Kết quả: Bus 004 Device 002: ID 0951:1666 Kingston Technology DataTraveler G4
```
```Kiểm tra driver nào đang bind với thiết bị
ls -la /sys/bus/usb/devices/4-1:1.0/driver
```
## ❓ Cần hỗ trợ?

1. **Không tìm thấy USB device**: Kiểm tra `lsusb | grep Kingston`
2. **Permission denied**: Chạy `sudo chmod 666 /dev/crypto0`  
3. **Module không nạp được**: Chạy `sudo apt-get install linux-headers-$(uname -r)`