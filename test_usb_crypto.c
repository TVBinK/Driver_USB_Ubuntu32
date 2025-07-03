/*
 * Chương trình test cho driver USB Caesar Cipher
 * Biên dịch: gcc -o test_usb_crypto test_usb_crypto.c
 * Sử dụng: ./test_usb_crypto
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEVICE_PATH "/dev/crypto0"
#define MAX_BUFFER_SIZE 1024

/* Hàm hiển thị menu */
void hien_thi_menu(void)
{
    printf("\n=== Test Driver USB Caesar Cipher ===\n");
    printf("1. Mã hóa văn bản\n");
    printf("2. Giải mã văn bản\n");
    printf("3. Test tự động\n");
    printf("4. Kiểm tra trạng thái thiết bị\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

/* Hàm kiểm tra thiết bị */
int kiem_tra_thiet_bi(void)
{
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("Lỗi: Không thể mở thiết bị %s\n", DEVICE_PATH);
        printf("Kiểm tra:\n");
        printf("1. Driver đã được nạp chưa? (lsmod | grep usb_crypto)\n");
        printf("2. Thiết bị USB đã kết nối chưa? (lsusb)\n");
        printf("3. File thiết bị có tồn tại không? (ls -la /dev/crypto*)\n");
        return -1;
    }
    close(fd);
    return 0;
}

/* Hàm gửi lệnh và nhận kết quả */
int gui_lenh_va_nhan_ket_qua(const char *lenh, char *ket_qua, size_t kich_thuoc_kq)
{
    int fd;
    ssize_t bytes_written, bytes_read, total_read = 0;
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        printf("Lỗi: Không thể mở thiết bị %s: %s\n", DEVICE_PATH, strerror(errno));
        return -1;
    }
    
    /* Gửi lệnh */
    bytes_written = write(fd, lenh, strlen(lenh));
    if (bytes_written < 0) {
        printf("Lỗi: Không thể ghi dữ liệu: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    printf("Đã gửi %zd byte: %s\n", bytes_written, lenh);
    
    /* Nhận kết quả */
    memset(ket_qua, 0, kich_thuoc_kq);
    while (total_read < kich_thuoc_kq - 1) {
        bytes_read = read(fd, ket_qua + total_read, kich_thuoc_kq - total_read - 1);
        if (bytes_read <= 0) {
            break;  /* Hết dữ liệu hoặc lỗi */
        }
        total_read += bytes_read;
    }
    
    close(fd);
    
    if (total_read > 0) {
        ket_qua[total_read] = '\0';
        printf("Đã nhận %zd byte: %s\n", total_read, ket_qua);
        return 0;
    } else {
        printf("Không nhận được dữ liệu\n");
        return -1;
    }
}

/* Hàm mã hóa văn bản */
void ma_hoa_van_ban(void)
{
    char van_ban[MAX_BUFFER_SIZE];
    char lenh[MAX_BUFFER_SIZE + 3];  /* "E:" + văn bản + null */
    char ket_qua[MAX_BUFFER_SIZE];
    
    printf("Nhập văn bản cần mã hóa: ");
    fgets(van_ban, sizeof(van_ban), stdin);
    
    /* Xóa ký tự newline */
    van_ban[strcspn(van_ban, "\n")] = '\0';
    
    if (strlen(van_ban) == 0) {
        printf("Văn bản trống!\n");
        return;
    }
    
    /* Tạo lệnh mã hóa */
    snprintf(lenh, sizeof(lenh), "E:%s", van_ban);
    
    printf("\nGửi lệnh mã hóa...\n");
    if (gui_lenh_va_nhan_ket_qua(lenh, ket_qua, sizeof(ket_qua)) == 0) {
        printf("Kết quả mã hóa: %s\n", ket_qua);
    }
}

/* Hàm giải mã văn bản */
void giai_ma_van_ban(void)
{
    char van_ban[MAX_BUFFER_SIZE];
    char lenh[MAX_BUFFER_SIZE + 3];  /* "D:" + văn bản + null */
    char ket_qua[MAX_BUFFER_SIZE];
    
    printf("Nhập văn bản cần giải mã: ");
    fgets(van_ban, sizeof(van_ban), stdin);
    
    /* Xóa ký tự newline */
    van_ban[strcspn(van_ban, "\n")] = '\0';
    
    if (strlen(van_ban) == 0) {
        printf("Văn bản trống!\n");
        return;
    }
    
    /* Tạo lệnh giải mã */
    snprintf(lenh, sizeof(lenh), "D:%s", van_ban);
    
    printf("\nGửi lệnh giải mã...\n");
    if (gui_lenh_va_nhan_ket_qua(lenh, ket_qua, sizeof(ket_qua)) == 0) {
        printf("Kết quả giải mã: %s\n", ket_qua);
    }
}

/* Hàm test tự động */
void test_tu_dong(void)
{
    char *cac_test[] = {
        "Hello World",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 
        "abcdefghijklmnopqrstuvwxyz",
        "Test 123 !@#",
        "Caesar Cipher Driver"
    };
    
    int so_test = sizeof(cac_test) / sizeof(cac_test[0]);
    char lenh[MAX_BUFFER_SIZE];
    char ket_qua_ma_hoa[MAX_BUFFER_SIZE];
    char ket_qua_giai_ma[MAX_BUFFER_SIZE];
    int i;
    
    printf("\n=== Test tự động ===\n");
    
    for (i = 0; i < so_test; i++) {
        printf("\nTest %d: \"%s\"\n", i + 1, cac_test[i]);
        
        /* Test mã hóa */
        snprintf(lenh, sizeof(lenh), "E:%s", cac_test[i]);
        printf("Mã hóa: ");
        if (gui_lenh_va_nhan_ket_qua(lenh, ket_qua_ma_hoa, sizeof(ket_qua_ma_hoa)) != 0) {
            printf("Lỗi mã hóa!\n");
            continue;
        }
        
        /* Test giải mã */
        snprintf(lenh, sizeof(lenh), "D:%s", ket_qua_ma_hoa);
        printf("Giải mã: ");
        if (gui_lenh_va_nhan_ket_qua(lenh, ket_qua_giai_ma, sizeof(ket_qua_giai_ma)) != 0) {
            printf("Lỗi giải mã!\n");
            continue;
        }
        
        /* Kiểm tra kết quả */
        if (strcmp(cac_test[i], ket_qua_giai_ma) == 0) {
            printf("✓ Test %d PASS\n", i + 1);
        } else {
            printf("✗ Test %d FAIL - Gốc: '%s', Kết quả: '%s'\n", 
                   i + 1, cac_test[i], ket_qua_giai_ma);
        }
    }
}

/* Hàm main */
int main(int argc, char *argv[])
{
    int lua_chon;
    char buffer[10];
    
    printf("Chương trình Test Driver USB Caesar Cipher\n");
    printf("==========================================\n");
    
    /* Kiểm tra thiết bị ban đầu */
    if (kiem_tra_thiet_bi() != 0) {
        printf("Thiết bị không sẵn sàng. Vui lòng kiểm tra và thử lại.\n");
        return 1;
    }
    
    printf("Thiết bị %s sẵn sàng!\n", DEVICE_PATH);
    
    while (1) {
        hien_thi_menu();
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("\nThoát chương trình.\n");
            break;
        }
        
        lua_chon = atoi(buffer);
        
        switch (lua_chon) {
            case 1:
                ma_hoa_van_ban();
                break;
                
            case 2:
                giai_ma_van_ban();
                break;
                
            case 3:
                test_tu_dong();
                break;
                
            case 4:
                if (kiem_tra_thiet_bi() == 0) {
                    printf("Thiết bị hoạt động bình thường.\n");
                } else {
                    printf("Thiết bị không phản hồi.\n");
                }
                break;
                
            case 0:
                printf("Thoát chương trình.\n");
                return 0;
                
            default:
                printf("Lựa chọn không hợp lệ!\n");
                break;
        }
    }
    
    return 0;
} 