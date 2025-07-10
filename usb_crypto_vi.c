/*
 * Driver USB Caesar Cipher cho Ubuntu 32bit
 * Triển khai driver USB hoạt động như thiết bị ký tự để mã hóa/giải mã dữ liệu
 * Sử dụng thuật toán Caesar cipher
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

/* Thông tin module */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("USB Crypto Driver");
MODULE_DESCRIPTION("Driver USB Caesar Cipher cho Ubuntu 32bit");
MODULE_VERSION("1.0");

/* Tham số module - độ dịch Caesar cipher */
static int dich = 13;
module_param(dich, int, 0644);
MODULE_PARM_DESC(dich, "Do dich Caesar cipher (0-25, mac dinh 13)");

/* Vendor ID và Product ID cho thiết bị USB Kingston DataTraveler */
#define USB_VENDOR_ID   0x0951
#define USB_PRODUCT_ID  0x1666

/* Thông tin thiết bị ký tự */
#define DEVICE_NAME     "usb_crypto"
#define DEVICE_CLASS    "usb"
#define MINOR_BASE      192
#define MAX_DEVICES     1
#define MAX_BUFFER_SIZE 65536  /* 64KB */

/* Cấu trúc dữ liệu driver */
struct usb_crypto_dev {
    struct usb_device *udev;
    struct usb_interface *interface;
    struct cdev cdev;
    dev_t dev_num;
    struct device *device;
    struct mutex mutex;
    char *g_buf_kq;        /* Buffer kết quả */
    size_t kich_thuoc_kq;  /* Kích thước kết quả */
    size_t vi_tri;         /* Vị trí đọc hiện tại */
    bool co_du_lieu;       /* Có dữ liệu để đọc không */
};

/* Biến toàn cục */
static struct class *usb_crypto_class;
static struct usb_crypto_dev *crypto_dev;
static DEFINE_MUTEX(global_mutex);

/* Bảng ID thiết bị USB được hỗ trợ */
static struct usb_device_id bang_id_thiet_bi[] = {
    { USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
    { }
};
MODULE_DEVICE_TABLE(usb, bang_id_thiet_bi);

/* Hàm mã hóa Caesar cipher */
static char ma_hoa_caesar(char ky_tu, int do_dich)
{
    if (ky_tu >= 'A' && ky_tu <= 'Z') {
        return (ky_tu - 'A' + do_dich) % 26 + 'A';
    } else if (ky_tu >= 'a' && ky_tu <= 'z') {
        return (ky_tu - 'a' + do_dich) % 26 + 'a';
    }
    return ky_tu;  /* Giữ nguyên ký tự không phải chữ cái */
}

/* Hàm giải mã Caesar cipher */
static char giai_ma_caesar(char ky_tu, int do_dich)
{
    int do_dich_nguoc = (26 - do_dich) % 26;
    return ma_hoa_caesar(ky_tu, do_dich_nguoc);
}

/* Hàm xử lý mã hóa/giải mã chuỗi */
static int xu_ly_du_lieu(const char *input, char **output, size_t kich_thuoc, bool ma_hoa)
{
    char *buffer;
    int i;
    
    buffer = kmalloc(kich_thuoc + 1, GFP_KERNEL);
    if (!buffer) {
        printk(KERN_ERR "usb_crypto: Khong the cap phat bo nho\n");
        return -ENOMEM;
    }
    
    for (i = 0; i < kich_thuoc; i++) {
        if (ma_hoa) {
            buffer[i] = ma_hoa_caesar(input[i], dich);
        } else {
            buffer[i] = giai_ma_caesar(input[i], dich);
        }
    }
    buffer[kich_thuoc] = '\0';
    
    *output = buffer;
    return 0;
}

/* Hàm mở thiết bị */
static int usb_crypto_open(struct inode *inode, struct file *file)
{
    struct usb_crypto_dev *dev;
    
    dev = container_of(inode->i_cdev, struct usb_crypto_dev, cdev);
    file->private_data = dev;
    
    printk(KERN_INFO "usb_crypto: Thiet bi duoc mo\n");
    return 0;
}

/* Hàm đóng thiết bị */
static int usb_crypto_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "usb_crypto: Thiet bi duoc dong\n");
    return 0;
}

/* Hàm ghi dữ liệu */
static ssize_t usb_crypto_write(struct file *file, const char __user *buffer, 
                               size_t count, loff_t *ppos)
{
    struct usb_crypto_dev *dev = file->private_data;
    char *kernel_buffer = NULL;
    char *data_part = NULL;
    char *result = NULL;
    bool ma_hoa = false;
    int ret = 0;
    size_t data_len;
    
    if (!dev) {
        return -ENODEV;
    }
    
    if (count > MAX_BUFFER_SIZE) {
        printk(KERN_ERR "usb_crypto: Du lieu qua lon (>64KB)\n");
        return -EINVAL;
    }
    
    mutex_lock(&dev->mutex);
    
    /* Cấp phát buffer cho dữ liệu từ userspace */
    kernel_buffer = kmalloc(count + 1, GFP_KERNEL);
    if (!kernel_buffer) {
        ret = -ENOMEM;
        goto out;
    }
    
    if (copy_from_user(kernel_buffer, buffer, count)) {
        ret = -EFAULT;
        goto out;
    }
    kernel_buffer[count] = '\0';
    
    /* Kiểm tra định dạng lệnh */
    if (count < 3 || (kernel_buffer[0] != 'E' && kernel_buffer[0] != 'D') || 
        kernel_buffer[1] != ':') {
        printk(KERN_ERR "usb_crypto: Dinh dang lenh khong hop le. Su dung E:<du_lieu> hoac D:<du_lieu>\n");
        ret = -EINVAL;
        goto out;
    }
    
    /* Xác định loại lệnh */
    ma_hoa = (kernel_buffer[0] == 'E');
    data_part = kernel_buffer + 2;  /* Bỏ qua "E:" hoặc "D:" */
    data_len = count - 2;
    
    /* Xử lý mã hóa/giải mã */
    ret = xu_ly_du_lieu(data_part, &result, data_len, ma_hoa);
    if (ret < 0) {
        goto out;
    }
    
    /* Giải phóng buffer kết quả cũ */
    if (dev->g_buf_kq) {
        kfree(dev->g_buf_kq);
    }
    
    /* Lưu kết quả mới */
    dev->g_buf_kq = result;
    dev->kich_thuoc_kq = data_len;
    dev->vi_tri = 0;
    dev->co_du_lieu = true;
    
    result = NULL;  /* Ngăn giải phóng trong cleanup */
    ret = count;
    
    printk(KERN_INFO "usb_crypto: Da xu ly %s %zu byte\n", 
           ma_hoa ? "ma hoa" : "giai ma", data_len);

out:
    if (kernel_buffer) {
        kfree(kernel_buffer);
    }
    if (result) {
        kfree(result);
    }
    mutex_unlock(&dev->mutex);
    return ret;
}

/* Hàm đọc dữ liệu */
static ssize_t usb_crypto_read(struct file *file, char __user *buffer, 
                              size_t count, loff_t *ppos)
{
    struct usb_crypto_dev *dev = file->private_data;
    size_t bytes_to_read;
    ssize_t ret = 0;
    
    if (!dev) {
        return -ENODEV;
    }
    
    mutex_lock(&dev->mutex);
    
    /* Kiểm tra có dữ liệu để đọc không */
    if (!dev->co_du_lieu || !dev->g_buf_kq) {
        printk(KERN_INFO "usb_crypto: Khong co du lieu de doc\n");
        ret = 0;
        goto out;
    }
    
    /* Kiểm tra đã đọc hết chưa */
    if (dev->vi_tri >= dev->kich_thuoc_kq) {
        ret = 0;  /* EOF */
        goto out;
    }
    
    /* Tính số byte cần đọc */
    bytes_to_read = min(count, dev->kich_thuoc_kq - dev->vi_tri);
    
    /* Copy dữ liệu ra userspace */
    if (copy_to_user(buffer, dev->g_buf_kq + dev->vi_tri, bytes_to_read)) {
        ret = -EFAULT;
        goto out;
    }
    
    /* Cập nhật vị trí đọc */
    dev->vi_tri += bytes_to_read;
    ret = bytes_to_read;
    
    printk(KERN_INFO "usb_crypto: Da doc %zu byte\n", bytes_to_read);

out:
    mutex_unlock(&dev->mutex);
    return ret;
}

/* Bảng file operations */
static const struct file_operations usb_crypto_fops = {
    .owner = THIS_MODULE,
    .open = usb_crypto_open,
    .release = usb_crypto_release,
    .read = usb_crypto_read,
    .write = usb_crypto_write,
};

/* Hàm kết nối thiết bị USB */
static int ham_ket_noi(struct usb_interface *interface, 
                      const struct usb_device_id *id)
{
    // Lấy thông tin USB device từ interface và lưu vào udev
    struct usb_device *udev = interface_to_usbdev(interface);
    int ret = 0;

    
    mutex_lock(&global_mutex);
    
    /* Kiểm tra đã có thiết bị chưa */
    if (crypto_dev) {
        printk(KERN_ERR "usb_crypto: Da co thiet bi dang hoat dong\n");
        ret = -EBUSY;
        goto out;
    }
    
    /* Cấp phát memory cho device structure */
    crypto_dev = kzalloc(sizeof(*crypto_dev), GFP_KERNEL);
    if (!crypto_dev) {
        ret = -ENOMEM;
        goto out;
    }
    
    /* Khởi tạo device structure */
    crypto_dev->udev = usb_get_dev(udev);        // Lưu USB device
    crypto_dev->interface = interface;           // Lưu USB interface
    mutex_init(&crypto_dev->mutex);              // Khởi tạo mutex
    crypto_dev->co_du_lieu = false;              // Chưa có dữ liệu
    crypto_dev->vi_tri = 0;                      // Vị trí đọc = 0
    crypto_dev->g_buf_kq = NULL;                 // Buffer kết quả = NULL
    crypto_dev->kich_thuoc_kq = 0;               // Kích thước = 0
    
    /* Đăng ký character device major/minor number*/
    ret = alloc_chrdev_region(&crypto_dev->dev_num, MINOR_BASE, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "usb_crypto: Khong the cap phat major number\n");
        goto error_alloc;
    }
    
    /* Khởi tạo character device */
    cdev_init(&crypto_dev->cdev, &usb_crypto_fops);
    crypto_dev->cdev.owner = THIS_MODULE;
    
    //thêm character device và major/minor number vào kernel
    ret = cdev_add(&crypto_dev->cdev, crypto_dev->dev_num, 1);
    if (ret) {
        printk(KERN_ERR "usb_crypto: Khong the them cdev\n");
        goto error_cdev;
    }
    
    /* Tạo device file */
    crypto_dev->device = device_create(usb_crypto_class, &interface->dev,
                                      crypto_dev->dev_num, NULL,
                                      "crypto%d", MINOR(crypto_dev->dev_num) - MINOR_BASE);
    if (IS_ERR(crypto_dev->device)) {
        ret = PTR_ERR(crypto_dev->device);
        printk(KERN_ERR "usb_crypto: Khong the tao device file\n");
        goto error_device;
    }
    
    /* Lưu con trỏ device vào USB interface */
    usb_set_intfdata(interface, crypto_dev);
    
    printk(KERN_INFO "usb_crypto: Thiet bi duoc tao thanh cong tai /dev/crypto%d\n",
           MINOR(crypto_dev->dev_num) - MINOR_BASE);
    
    goto out;

error_device:
    cdev_del(&crypto_dev->cdev);
error_cdev:
    unregister_chrdev_region(crypto_dev->dev_num, 1);
error_alloc:
    usb_put_dev(crypto_dev->udev);
    kfree(crypto_dev);
    crypto_dev = NULL;
out:
    mutex_unlock(&global_mutex);
    return ret;
}

/* Hàm ngắt kết nối thiết bị USB */
static void ngat_ket_noi(struct usb_interface *interface)
{
    // Lấy con trỏ đến struct usb_crypto_dev từ interface
    struct usb_crypto_dev *dev = usb_get_intfdata(interface);
    
    printk(KERN_INFO "usb_crypto: Thiet bi USB ngat ket noi\n");
    
    mutex_lock(&global_mutex);
    
    if (!dev) {
        goto out;
    }
    
    /* Xóa device file */
    device_destroy(usb_crypto_class, dev->dev_num);
    
    /* Xóa character device */
    cdev_del(&dev->cdev);
    
    /* Giải phóng major/minor number */
    unregister_chrdev_region(dev->dev_num, 1);
    
    /* Giải phóng buffer */
    mutex_lock(&dev->mutex);
    if (dev->g_buf_kq) {
        kfree(dev->g_buf_kq);
        dev->g_buf_kq = NULL;
    }
    mutex_unlock(&dev->mutex);
    
    /* Giải phóng USB device reference */
    usb_put_dev(dev->udev);
    
    /* Giải phóng device structure */
    kfree(dev);
    crypto_dev = NULL;
    
    usb_set_intfdata(interface, NULL);
    
    printk(KERN_INFO "usb_crypto: Thiet bi da duoc giai phong\n");

out:
    mutex_unlock(&global_mutex);
}

/* USB driver structure */
static struct usb_driver usb_crypto_driver = {
    .name = DEVICE_NAME,
    .id_table = bang_id_thiet_bi,
    .probe = ham_ket_noi,
    .disconnect = ngat_ket_noi,
};

/* Hàm khởi tạo module */
static int __init usb_crypto_init(void)
{
    int ret;
    
    printk(KERN_INFO "usb_crypto: Khoi tao driver USB Caesar Cipher\n");
    printk(KERN_INFO "usb_crypto: Do dich Caesar: %d\n", dich);
    
    /* Kiểm tra tham số độ dịch */
    if (dich < 0 || dich > 25) {
        printk(KERN_ERR "usb_crypto: Do dich khong hop le (%d). Phai tu 0-25\n", dich);
        return -EINVAL;
    }
    
    /* Tạo device class */
    usb_crypto_class = class_create(THIS_MODULE, DEVICE_CLASS);
    if (IS_ERR(usb_crypto_class)) {
        printk(KERN_ERR "usb_crypto: Khong the tao device class\n");
        return PTR_ERR(usb_crypto_class);
    }
    
    /* Đăng ký USB driver */
    ret = usb_register(&usb_crypto_driver);
    if (ret) {
        printk(KERN_ERR "usb_crypto: Khong the dang ky USB driver\n");
        class_destroy(usb_crypto_class);
        return ret;
    }
    
    printk(KERN_INFO "usb_crypto: Driver da duoc nap thanh cong\n");
    return 0;
}

/* Hàm dọn dẹp module */
static void __exit usb_crypto_exit(void)
{
    printk(KERN_INFO "usb_crypto: Dang giai phong driver\n");
    
    /* Hủy đăng ký USB driver */
    usb_deregister(&usb_crypto_driver);
    
    /* Xóa device class */
    class_destroy(usb_crypto_class);
    
    printk(KERN_INFO "usb_crypto: Driver da duoc giai phong\n");
}

module_init(usb_crypto_init);
module_exit(usb_crypto_exit); 