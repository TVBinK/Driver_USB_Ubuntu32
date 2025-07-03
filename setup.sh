#!/bin/bash
#
# Script cài đặt tự động cho Driver USB Caesar Cipher
# Sử dụng: chmod +x setup.sh && ./setup.sh
#

set -e  # Dừng script khi có lỗi

# Màu sắc cho output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Hàm in màu
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Hàm kiểm tra quyền root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_warning "Script đang chạy với quyền root. Một số tác vụ có thể không cần thiết."
    fi
}

# Hàm kiểm tra hệ thống
check_system() {
    print_info "Kiểm tra hệ thống..."
    
    # Kiểm tra OS
    if [[ ! -f /etc/os-release ]]; then
        print_error "Không thể xác định hệ điều hành"
        exit 1
    fi
    
    source /etc/os-release
    print_info "Hệ điều hành: $PRETTY_NAME"
    
    # Kiểm tra architecture  
    ARCH=$(uname -m)
    print_info "Kiến trúc: $ARCH"
    
    if [[ "$ARCH" != "i686" && "$ARCH" != "i386" ]]; then
        print_warning "Driver được thiết kế cho Ubuntu 32bit. Kiến trúc hiện tại: $ARCH"
        read -p "Bạn có muốn tiếp tục? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    # Kiểm tra kernel version
    KERNEL_VERSION=$(uname -r)
    print_info "Kernel version: $KERNEL_VERSION"
}

# Hàm cài đặt dependencies
install_dependencies() {
    print_info "Cài đặt dependencies..."
    
    # Cập nhật package list
    print_info "Cập nhật package list..."
    sudo apt-get update
    
    # Cài đặt kernel headers
    print_info "Cài đặt kernel headers..."
    sudo apt-get install -y linux-headers-$(uname -r)
    
    # Cài đặt build tools
    print_info "Cài đặt build-essential..."
    sudo apt-get install -y build-essential
    
    # Cài đặt các tools khác
    print_info "Cài đặt các tools bổ sung..."
    sudo apt-get install -y gcc make dkms usbutils
    
    print_success "Dependencies đã được cài đặt"
}

# Hàm kiểm tra dependencies
check_dependencies() {
    print_info "Kiểm tra dependencies..."
    
    local missing_deps=()
    
    # Kiểm tra gcc
    if ! command -v gcc &> /dev/null; then
        missing_deps+=("gcc")
    fi
    
    # Kiểm tra make
    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi
    
    # Kiểm tra kernel headers
    if [[ ! -d "/lib/modules/$(uname -r)/build" ]]; then
        missing_deps+=("linux-headers-$(uname -r)")
    fi
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_warning "Thiếu dependencies: ${missing_deps[*]}"
        read -p "Bạn có muốn cài đặt tự động? (Y/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Nn]$ ]]; then
            install_dependencies
        else
            print_error "Không thể tiếp tục mà không có dependencies"
            exit 1
        fi
    else
        print_success "Tất cả dependencies đã sẵn sàng"
    fi
}

# Hàm build driver
build_driver() {
    print_info "Building driver..."
    
    if [[ ! -f "usb_crypto_vi.c" ]]; then
        print_error "Không tìm thấy file usb_crypto_vi.c"
        exit 1
    fi
    
    if [[ ! -f "Makefile" ]]; then
        print_error "Không tìm thấy Makefile"
        exit 1
    fi
    
    # Clean previous build
    make clean 2>/dev/null || true
    
    # Build module
    if make; then
        print_success "Driver đã được build thành công"
    else
        print_error "Build driver thất bại"
        exit 1
    fi
}

# Hàm build test program
build_test_program() {
    print_info "Building test program..."
    
    if [[ ! -f "test_usb_crypto.c" ]]; then
        print_error "Không tìm thấy file test_usb_crypto.c"
        exit 1
    fi
    
    if gcc -o test_usb_crypto test_usb_crypto.c; then
        print_success "Test program đã được build thành công"
    else
        print_error "Build test program thất bại"
        exit 1
    fi
}

# Hàm kiểm tra USB device
check_usb_device() {
    print_info "Kiểm tra thiết bị USB..."
    
    if lsusb | grep -q "0951:1666"; then
        print_success "Tìm thấy thiết bị USB Kingston DataTraveler (0951:1666)"
        return 0
    else
        print_warning "Không tìm thấy thiết bị USB Kingston DataTraveler (0951:1666)"
        print_info "Danh sách thiết bị USB:"
        lsusb | grep -i kingston || print_info "Không có thiết bị Kingston nào"
        return 1
    fi
}

# Hàm cài đặt driver
install_driver() {
    print_info "Cài đặt driver..."
    
    if [[ ! -f "usb_crypto_vi.ko" ]]; then
        print_error "File module usb_crypto_vi.ko không tồn tại. Vui lòng build trước."
        exit 1
    fi
    
    # Gỡ module cũ nếu có
    if lsmod | grep -q "usb_crypto_vi"; then
        print_info "Gỡ module cũ..."
        sudo rmmod usb_crypto_vi || print_warning "Không thể gỡ module cũ"
    fi
    
    # Cài đặt module mới
    print_info "Nạp module mới..."
    if sudo insmod usb_crypto_vi.ko; then
        print_success "Module đã được nạp thành công"
        
        # Kiểm tra module
        if lsmod | grep -q "usb_crypto_vi"; then
            print_success "Module usb_crypto_vi đang hoạt động"
        fi
        
        # Xem kernel messages
        print_info "Kernel messages:"
        dmesg | tail -5 | grep usb_crypto || print_info "Không có message nào"
        
    else
        print_error "Không thể nạp module"
        exit 1
    fi
}

# Hàm test driver
test_driver() {
    print_info "Test driver..."
    
    # Kiểm tra device file
    if [[ -e "/dev/crypto0" ]]; then
        print_success "Device file /dev/crypto0 tồn tại"
        ls -la /dev/crypto*
    else
        print_warning "Device file /dev/crypto0 không tồn tại"
        print_info "Có thể driver chưa nhận diện được thiết bị USB"
    fi
    
    # Chạy test program nếu có
    if [[ -x "./test_usb_crypto" ]]; then
        print_info "Chạy test program..."
        print_info "Sử dụng Ctrl+C để thoát test program"
        ./test_usb_crypto
    else
        print_warning "Test program chưa được build"
    fi
}

# Hàm hiển thị trạng thái
show_status() {
    print_info "Trạng thái hệ thống:"
    
    echo "=== Module Status ==="
    lsmod | grep usb_crypto || echo "Module không được nạp"
    
    echo "=== USB Devices ==="
    lsusb | grep -i kingston || echo "Không có thiết bị Kingston"
    
    echo "=== Device Files ==="
    ls -la /dev/crypto* 2>/dev/null || echo "Không có device file nào"
    
    echo "=== Recent Kernel Messages ==="
    dmesg | tail -10 | grep usb_crypto || echo "Không có message nào"
}

# Hàm dọn dẹp
cleanup() {
    print_info "Dọn dẹp..."
    
    # Gỡ module
    if lsmod | grep -q "usb_crypto_vi"; then
        print_info "Gỡ module..."
        sudo rmmod usb_crypto_vi || print_warning "Không thể gỡ module"
    fi
    
    # Dọn dẹp build files
    print_info "Dọn dẹp build files..."
    make clean 2>/dev/null || true
    rm -f test_usb_crypto
    
    print_success "Dọn dẹp hoàn thành"
}

# Hàm hiển thị menu
show_menu() {
    echo "=== USB Crypto Driver Setup ==="
    echo "1. Kiểm tra hệ thống"
    echo "2. Cài đặt dependencies"
    echo "3. Build driver"
    echo "4. Build test program"
    echo "5. Cài đặt driver"
    echo "6. Test driver"
    echo "7. Kiểm tra trạng thái"
    echo "8. Dọn dẹp"
    echo "9. Cài đặt hoàn chỉnh (1-6)"
    echo "0. Thoát"
    echo "=========================="
}

# Hàm main
main() {
    print_info "USB Crypto Driver Setup Script"
    print_info "=============================="
    
    # Kiểm tra quyền
    check_root
    
    if [[ $# -eq 0 ]]; then
        # Interactive mode
        while true; do
            echo
            show_menu
            read -p "Chọn (0-9): " choice
            
            case $choice in
                1) check_system ;;
                2) install_dependencies ;;
                3) build_driver ;;
                4) build_test_program ;;
                5) install_driver ;;
                6) test_driver ;;
                7) show_status ;;
                8) cleanup ;;
                9) 
                    check_system
                    check_dependencies
                    build_driver
                    build_test_program
                    check_usb_device
                    install_driver
                    show_status
                    ;;
                0) 
                    print_info "Thoát script"
                    exit 0
                    ;;
                *) 
                    print_error "Lựa chọn không hợp lệ"
                    ;;
            esac
        done
    else
        # Command line mode
        case $1 in
            "install")
                check_system
                check_dependencies
                build_driver
                build_test_program
                install_driver
                ;;
            "test") test_driver ;;
            "status") show_status ;;
            "clean") cleanup ;;
            "help"|"-h"|"--help")
                echo "Sử dụng: $0 [install|test|status|clean|help]"
                echo "Hoặc chạy không tham số để vào interactive mode"
                ;;
            *)
                print_error "Tham số không hợp lệ: $1"
                echo "Sử dụng: $0 help để xem hướng dẫn"
                exit 1
                ;;
        esac
    fi
}

# Chạy script
main "$@" 