/**
 * @file ioctl-2.c
 */
#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <asm/errno.h>

#include "ioctl-2.h"

#define SUCCESS 0
#define BUF_LEN 80

enum {
	CDEV_NOT_USED,
	CDEV_EXCLUSIVE_OPEN,
};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char message[BUF_LEN + 1];

static struct class *cls;

/**
 * @brief open()が呼び出されたときの処理
 */
static int device_open(struct inode *inode, struct file *file) {
	pr_info("device_open(%p)\n", file);

	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/**
 * @brief close()が呼び出されたときの処理
 */
static int device_release(struct inode *inode, struct file *file) {
	pr_info("device_release(%p,%p)\n", inode, file);
	
	module_put(THIS_MODULE);

	return SUCCESS;
}

/**
 * @brief read()が呼び出されたときの処理
 */
static ssize_t device_read(struct file *file, char __user *buffer,
						   size_t length, loff_t *offset)
{
	/* バッファに書き込まれたバイト数 */
	int bytes_read = 0;

	const char *message_ptr = message;

	if (!*(message_ptr + *offset)) {
		*offset = 0;
		return 0;
	}

	message_ptr += *offset;

	/* データをバッファに入れる */
	while (length && *message_ptr) {
		/* put_userでユーザ空間へ1バイトずつコピーする */
		put_user(*(message_ptr++), buffer++);
		length--;
		bytes_read++;
	}

	pr_info("Read %d bytes, %ld left\n", bytes_read, length);

	*offset += bytes_read;

	return bytes_read;
}

/**
 * @brief write()呼び出されたときの処理
 */
static ssize_t device_write(struct file *file, const char __user *buffer,
							size_t length, loff_t *offset)
{
	int i;

	pr_info("device_write(%p,%p,%ld)", file, buffer, length);

	for (i = 0; i < length && i < BUF_LEN; i++) {
		get_user(message[i], buffer + i);
	}

	return 0;
}

/**
 * @brief ioctl()が呼び出されたときの処理
 */
static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
	int i;
	long ret = SUCCESS;

	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	switch (ioctl_num) {
	case IOCTL_SET_MSG: {
		char __user *tmp = (char __user *)ioctl_param;
		char ch;

		get_user(ch, tmp);
		for (i = 0; ch && i < BUF_LEN; i++, tmp++) {
			get_user(ch, tmp);
		}

		device_write(file, (char __user *)ioctl_param, i, NULL);
		break;
	}
	case IOCTL_GET_MSG: {
		loff_t offset = 0;

		i = device_read(file, (char __user *)ioctl_param, 99, &offset);

		put_user('\0', (char __user *)ioctl_param + i);
		break;
	}
	case IOCTL_GET_NTH_BYTE:
		ret = (long)message[ioctl_param];
		break;
	}

	atomic_set(&already_open, CDEV_NOT_USED);

	return ret;
}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init chardev_init(void) {
	int ret_val = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &fops);

	if (ret_val < 0) {
		pr_alert("%s failed with %d\n", "Sorry, registering the character device ", ret_val);

		return ret_val;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	cls = class_create(DEVICE_FILE_NAME);
#else
	cls = class_create(THIS_MODULE, DEVICE_FILE_NAME);
#endif

	device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);

	pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit chardev_exit(void) {
	device_destroy(cls, MKDEV(MAJOR_NUM, 0));
	class_destroy(cls);

	unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");