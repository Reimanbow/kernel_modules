/**
 * @file cdev-time.c
 * 
 * cat /dev/time を実行すると, 現在の日時を取得できるデバイス
 */
#include "cdev-time.h"

//! 割り当てられるメジャー番号
int major;

//! デバイスへの複数アクセスを防ぐための状態を持つ
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

//! device_create()に使用するクラス構造体
static struct class *cls;

//! コールバック関数を登録するためのfile_operations構造体

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
 * @brief readが呼び出されたときの処理
 */
static ssize_t device_read(struct file *file, char __user *buffer,
						   size_t length, loff_t *offset)
{
	char time_str[64];
	time64_t now = ktime_get_real_seconds();
	struct tm tm_time;

	time64_to_tm(now, 0, &tm_time);
	snprintf(time_str, sizeof(time_str), "%04ld-%02d-%02d %02d:%02d:%02d\n",
			tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
			tm_time.tm_hour + 9, tm_time.tm_min, tm_time.tm_sec);
	
	return simple_read_from_buffer(buffer, length, offset, time_str, strlen(time_str));
}

/**
 * @struct file_operations
 * @brief コールバック関数を登録する
 */
struct file_operations cdev_fops = {
	.open = device_open,
	.release = device_release,
	.read = device_read,
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