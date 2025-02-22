/**
 * @file reverse.c
 * 
 * 書き込まれた文字列を逆順にして返すキャラクタデバイス
 * 書き込み例 -- sudo bash -c 'echo "hello" > /dev/reverse'
 * 読み込み例 -- sudo cat /dev/reverse
 */
#include "reverse.h"

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <asm/errno.h>

//! デバイスドライバに割り当てるメジャー番号
int major;

//! デバイスへの複数アクセスを防ぐための状態を持つ
atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

//! デバイスが受け取るメッセージ
char msg[BUF_LEN + 1];

//! デバイスが反転させるメッセージ
char rev_msg[BUF_LEN + 1];

//! device_create()に使用するクラス構造体
struct class *cls;

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
 * @brief カーネルモジュールの起動処理. キャラクタデバイスの作成を行う
 */
static int __init chardev_init(void) {
	major = register_chrdev(0, DEVICE_NAME, &cdev_fops);

	if (major < 0) {
		pr_alert("Registering char device failed with %d\n", major);
		return major;
	}

	pr_info("assigned major number %d.\n", major);

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
 * @brief カーネルモジュールの終了処理. キャラクタデバイスの削除を行う
 */
static void __exit chardev_exit(void) {
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME);
}

/**
 * @brief プロセスがデバイスファイルを開くときの処理
 */
static int device_open(struct inode *inode, struct file *file) {
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/**
 * @brief プロセスがデバイスファイルを閉じるときの処理
 */
static int device_release(struct inode *inode, struct file *file) {
	atomic_set(&already_open, CDEV_NOT_USED);

	module_put(THIS_MODULE);

	return SUCCESS;
}

/**
 * @brief プロセスがデバイスファイルから読み込もうとしたときの処理
 */
static ssize_t device_read(struct file *filp, char __user *buffer,
					       size_t length, loff_t *offset) {
	size_t msg_len = strlen(rev_msg);  // 逆順メッセージの長さ

	// オフセットがメッセージの長さ以上なら、読み取るデータはない
	if (*offset >= msg_len) {
		return 0;  // EOF（End of File）
	}

	size_t bytes_read = msg_len - *offset;  // 読み取るデータの残りの長さ
	if (length < bytes_read) {
		bytes_read = length;  // ユーザーが要求した長さに合わせる
	}

	// ユーザ空間にデータをコピー
	if (copy_to_user(buffer, rev_msg + *offset, bytes_read) != 0) {
		return -EFAULT;  // copy_to_user() 失敗時にエラー返す
	}

	*offset += bytes_read;  // オフセットを更新して次回読み込み位置を設定

	return bytes_read;  // 実際に読み込んだバイト数を返す
}


/**
 * @brief プロセスがデバイスファイルに書き込むときの処理
 */
static ssize_t device_write(struct file *filp, const char __user *buffer,
							size_t length, loff_t *offset) {
	size_t bytes_write = (length < BUF_LEN) ? length : BUF_LEN;  // 書き込むデータの長さを制限

	/* ユーザ空間からメッセージをコピーする */
	if (copy_from_user(msg, buffer, bytes_write) != 0) {
		return -EFAULT;  // copy_from_user() 失敗時にエラー返す
	}

	msg[bytes_write] = '\0';  // 文字列の終端を追加

	/* 逆順に格納 */
	for (size_t i = 0; i < bytes_write; i++) {
		rev_msg[i] = msg[bytes_write - i - 1];
	}
	rev_msg[bytes_write] = '\0';  // 逆順データにも終端を追加

	return bytes_write;  // 実際に書き込んだバイト数を返す
}


module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Reimanbow");