# Makefile cho driver USB Caesar Cipher trên Ubuntu 32bit
# Sử dụng: make để build, make clean để dọn dẹp

# Tên module
MODULE_NAME := usb_crypto_vi

# Kernel source directory (tự động detect)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Danh sách object files
obj-m := $(MODULE_NAME).o

# Target mặc định
all:
	@echo "Building USB Crypto Driver for Ubuntu 32bit..."
	make -C $(KERNEL_DIR) M=$(PWD) modules

# Target cài đặt module
install: all
	@echo "Installing module..."
	sudo insmod $(MODULE_NAME).ko
	@echo "Module installed successfully"
	@echo "Device will appear at /dev/cryptoX when USB device is connected"

# Target gỡ bỏ module  
uninstall:
	@echo "Removing module..."
	-sudo rmmod $(MODULE_NAME)
	@echo "Module removed"

# Target kiểm tra trạng thái module
status:
	@echo "Module status:"
	lsmod | grep $(MODULE_NAME) || echo "Module not loaded"
	@echo ""
	@echo "USB devices:"
	lsusb | grep -i kingston || echo "No Kingston USB device found"
	@echo ""
	@echo "Device files:"
	ls -la /dev/crypto* 2>/dev/null || echo "No crypto device files found"

# Target xem kernel log
log:
	@echo "Recent kernel messages:"
	dmesg | tail -20 | grep usb_crypto

# Target dọn dẹp
clean:
	@echo "Cleaning build files..."
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f Module.symvers modules.order

# Target cài đặt với tham số độ dịch tùy chỉnh
install-custom:
	@echo "Installing with custom shift parameter..."
	@read -p "Enter shift value (0-25): " shift; \
	sudo insmod $(MODULE_NAME).ko dich=$$shift
	@echo "Module installed with custom shift"

# Target build và test
test: all
	@echo "Building and testing driver..."
	sudo dmesg -C  # Clear kernel log
	make install
	sleep 2
	make status
	make log

# Target giúp đỡ
help:
	@echo "Available targets:"
	@echo "  all          - Build the driver module"
	@echo "  install      - Install module with default settings"
	@echo "  install-custom - Install module with custom shift value"
	@echo "  uninstall    - Remove the module"
	@echo "  status       - Check module and device status"
	@echo "  log          - Show recent kernel messages"
	@echo "  test         - Build, install and test"
	@echo "  clean        - Clean build files"
	@echo "  help         - Show this help"

.PHONY: all install uninstall status log clean install-custom test help 