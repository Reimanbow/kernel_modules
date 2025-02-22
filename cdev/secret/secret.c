/**
 * @file secret.c
 * 
 * write()で書き込んだデータを保存し, 1回だけread()で読み出せるデバイス
 */
#include "secret.h"

//! 割り当てられるメジャー番号
int major;

//! データを受け取るバッファ
char dev_buffer[MAX_BUFFER_SIZE];

//! バッファのサイズ
size_t buffer_size = 0;

//! デバイスへの複数アクセスを防ぐための状態を持つ
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

//! device_create()に使用するクラス構造体
static struct class *cls;

/**
 * @brief open()が呼び出されたときの処理
 */
static int device_open(struct inode *inode, struct file *file) {
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	try_module_get(THIS_MODULE);

	return 0;
}

/**
 * @brief close()が呼び出されたときの処理
 */
static int device_release(struct inode *inode, struct file *file) {
	atomic_set(&already_open, CDEV_NOT_USED);

	module_put(THIS_MODULE);

	return 0;
}

/**
 * @brief read()が呼び出されたときの処理
 */
static ssize_t device_read(struct file *file, char __user *buffer,
						   size_t length, loff_t *offset)
{
	if (*offset || buffer_size == 0) {
		pr_debug("dev read: END\n");
		*offset = 0;
		return 0;
	}
	
	size_t read_buffer_size = min(buffer_size, length);
	if (copy_to_user(buffer, dev_buffer, read_buffer_size)) {
		return -EFAULT;
	}
	*offset += read_buffer_size;

	pr_debug("read %lu bytes\n", read_buffer_size);

	/* カーネル空間のバッファを空にする */
	snprintf(dev_buffer, buffer_size, "");
	buffer_size = 0;

	return read_buffer_size;
}

/**
 * @brief write()が呼び出されたときの処理
 */
static ssize_t device_write(struct file *file, const char __user *buffer,
							size_t length, loff_t *offset)
{
	buffer_size = min(MAX_BUFFER_SIZE, length);

	if (copy_from_user(dev_buffer, buffer, buffer_size)) {
		return -EFAULT;
	}
	*offset += buffer_size;

	pr_debug("write %lu bytes\n", buffer_size);
	return buffer_size;
}

/**
 * @struct file_operations
 * @brief コールバック関数を登録する
 */
struct file_operations cdev_fops = {
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write,
};

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init chardev_init(void) {
	major = register_chrdev(0, DEVICE_NAME, &cdev_fops);

	if (major < 0) {
		pr_alert("Registering char device failed with %d\n", major);
		return major;
	}

	pr_info("assigned major number %d\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	cls = class_create(DEVICE_NAME);
#else
	cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif

	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	pr_info("Device created on /dev/%s\n", DEVICE_NAME);

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ処理
 */
static void __exit chardev_exit(void) {
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Reimanbow");