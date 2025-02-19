/**
 * @file chardev.c
 * 
 * devファイルから何回読み込んだかを示す, 読み取り専用のキャラクタデバイスを作成する
 */
#include "chardev.h"

#include <linux/atomic.h>	// アトミック操作（排他制御なしにデータを変更する）
#include <linux/cdev.h>		// キャラクタデバイス（cdev）の管理を行う
#include <linux/delay.h>	// 遅延処理を行う関数を提供する
#include <linux/device.h>	// デバイスドライバの管理を行う
#include <linux/init.h>		// カーネルモジュールの初期化, クリーンアップ関連のマクロや関数を提供する
#include <linux/kernel.h>	// カーネル関連の汎用的なマクロ, 関数を提供する
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uaccess.h>	// ユーザ空間とのデータのやりとり（コピー）を行う回数を提供する
#include <linux/version.h>	// カーネルのバージョンを判定するためのマクロを提供する

#include <asm/errno.h>		// カーネル内で使用されるエラーコードを提供する

//! デバイスドライバに割り当てられるメジャー番号
int major;

//! デバイスへの複数アクセスを防ぐために使用
atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

//! デバイスが返すメッセージ
char msg[BUF_LEN + 1];

//! device_create()に使用するクラス構造体
struct class *cls;

/**
 * @struct file_operations
 * @brief コールバック関数を登録するためのfile_operations構造体の宣言
 */
struct file_operations chardev_fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

/**
 * @brief デバイスの初期化
 */
static int __init chardev_init(void) {
	/**
	 * カーネルにキャラクタデバイスを登録し, メジャー番号を取得 
	 * 0を渡すと動的にメジャー番号が割り当てられる
	 */
	major = register_chrdev(0, DEVICE_NAME, &chardev_fops);

	if (major < 0) {
		pr_alert("Registering char device failed with %d\n", major);
		return major;
	}

	pr_info("I was assigned major number %d.\n", major);
	
	/* /dev/chardevを作成し, ユーザがデバイスを簡単にアクセスできるようにする */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	cls = class_create(DEVICE_NAME);
#else
	cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	pr_info("Device created on /dev/%s\n", DEVICE_NAME);

	return SUCCESS;
}

/**
 * @brief デバイスの終了. /dev/chardevを削除し, キャラクタデバイスの登録を解除
 */
static void __exit chardev_exit(void) {
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME);
}

/**
 * @brief プロセスがsudo cat /dev/chardevのようにデバイスファイルを開こうとしたときに呼び出される
 */
static int device_open(struct inode *inode, struct file *file) {
	static int counter = 0;

	/* 排他制御を行い, デバイスを同時に1つのプロセスしか開けないようにする */
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	sprintf(msg, "I already told you %d times Hello world\n", counter++);
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/**
 * @brief デバイスファイルを閉じるときに呼び出される
 */
static int device_release(struct inode *inode, struct file *file) {
	/* 排他制御を解除 */
	atomic_set(&already_open, CDEV_NOT_USED);

	/* 一度オープンしたら, そのモジュールは決して削除されない */
	module_put(THIS_MODULE);

	return SUCCESS;
}

/**
 * すでにdevファイルをオープンしているプロセスが, そのdevファイルから読み込もうとしたときに呼び出される
 */
static ssize_t device_read(struct file *filp,
						   char __user *buffer,
						   size_t length,
						   loff_t *offset) {
	/* 実際にバッファに書き込まれたバイト数 */
	int bytes_read = 0;
	const char *msg_ptr = msg;

	if (!*(msg_ptr + *offset)) {
		*offset = 0;
		return 0;
	}

	msg_ptr += *offset;

	/* 実際にデータをバッファに入れる */
	while (length && *msg_ptr) {
		put_user(*(msg_ptr++), buffer++);
		length--;
		bytes_read++;
	}

	*offset += bytes_read;

	/* バッファに入れられたバイト数を返す */
	return bytes_read;
}

/**
 * @brief プロセスがdevファイルに書き込むときに呼び出される
 */
static ssize_t device_write(struct file *filp, const char __user *buff,
							size_t len, loff_t *off) {
	pr_alert("Sorry, this operation is not supported.\n");
	return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");