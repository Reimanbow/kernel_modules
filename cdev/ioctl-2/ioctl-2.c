/**
 * @file ioctl-2.c
 * 
 * キャラクタデバイスドライバを実装して, ioctlを用いたメッセージのやり取りができる
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

/**
 * @enum 排他制御のための列挙体
 */
enum {
	CDEV_NOT_USED,
	CDEV_EXCLUSIVE_OPEN,
};

//! デバイスの排他制御を行う(初期状態は未使用)
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

//! デバイス内のメッセージを格納するバッファ
static char message[BUF_LEN + 1];

//! デバイスのクラスに使用
static struct class *cls;

/**
 * @brief open()が呼び出されたときの処理
 */
static int device_open(struct inode *inode, struct file *file) {
	pr_info("device_open(%p)\n", file);

	/* カーネルモジュールの参照カウントを増やす. モジュールがアンロードされないようにする */
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/**
 * @brief close()が呼び出されたときの処理
 */
static int device_release(struct inode *inode, struct file *file) {
	pr_info("device_release(%p,%p)\n", inode, file);
	
	/* カーネルモジュールの参照カウントを減らす. アンロードを可能にする */
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

	/* messageのポインタを取得する */
	const char *message_ptr = message;

	/**
	 * データの終端チェック
	 * *offset位置のデータがNULLなら, 読み取り終了(0を返す)
	 * *offset = 0; でオフセットをリセット
	 */
	if (!*(message_ptr + *offset)) {
		*offset = 0;
		return 0;
	}

	message_ptr += *offset;

	/**
	 * offset位置からbufferへデータをコピー
	 * lengthバイト or messageの終端まで繰り返し
	 */
	while (length && *message_ptr) {
		/* put_userでユーザ空間へ1バイトずつコピーする */
		put_user(*(message_ptr++), buffer++);
		length--;
		bytes_read++;
	}

	pr_info("Read %d bytes, %ld left\n", bytes_read, length);

	/* オフセットを更新 */
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

	/* ユーザ空間のbufferからmessageにデータをコピー */
	for (i = 0; i < length && i < BUF_LEN; i++) {
		/* ユーザ空間のデータをカーネル空間へコピー */
		get_user(message[i], buffer + i);
	}

	return 0;
}

/**
 * @brief ioctl()が呼び出されたときの処理. 通常のread()やwrite()ではできないデバイス固有の操作を実現できる
 * 
 * @param file file構造体
 * @param ioctl_num ioctlのコマンド番号(ioctl-2.hで定義)
 * @param ioctl_param ioctlの引数(ユーザ空間から渡されるデータ)
 */
static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
	int i;
	long ret = SUCCESS;

	/* 排他制御: デバイスが既にオープンされていないかチェック */
	if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) {
		return -EBUSY;
	}

	/* 受け取ったioctl_numに応じて, 異なる動作を実行する */
	switch (ioctl_num) {
	/* メッセージを設定 */
	case IOCTL_SET_MSG: {
		/* ioctl_paramはユーザ空間の文字列のポインタであるのでキャストする(char __user *tmp) */
		char __user *tmp = (char __user *)ioctl_param;
		char ch;

		/* tmpの最初の文字をカーネル空間へのコピー */
		get_user(ch, tmp);
		/* BUF_LENまでの文字列を取得 */
		for (i = 0; ch && i < BUF_LEN; i++, tmp++) {
			get_user(ch, tmp);
		}

		/* 取得したデータをmessageに格納する */
		device_write(file, (char __user *)ioctl_param, i, NULL);
		break;
	}
	/* メッセージを取得 */
	case IOCTL_GET_MSG: {
		loff_t offset = 0;

		/**
		 * ioctl_paramは, ユーザ空間のバッファのポインタであるのでキャストする
		 * device_read()を呼び出して, messageの内容をioctl_paramにコピー
		 */
		i = device_read(file, (char __user *)ioctl_param, 99, &offset);

		/* 取得したデータの最後に'\0'を追加する */
		put_user('\0', (char __user *)ioctl_param + i);
		break;
	}
	/* メッセージのnバイト目を取得 */
	case IOCTL_GET_NTH_BYTE:
		/* ioctl_paramは, 取得したいバイト位置(n) */
		/* message[n]の値をlong型で返す */
		ret = (long)message[ioctl_param];
		break;
	}

	/* デバイスの使用状態をCDEV_NOT_USEDに戻す */
	atomic_set(&already_open, CDEV_NOT_USED);

	return ret;
}

/**
 * @struct file_operations
 * @brief キャラクタデバイスが対応する操作を定義する構造体
 */
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
	/* キャラクタデバイスを登録する */
	int ret_val = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &fops);

	if (ret_val < 0) {
		pr_alert("%s failed with %d\n", "Sorry, registering the character device ", ret_val);

		return ret_val;
	}

	/* デバイスクラスの作成 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	cls = class_create(DEVICE_FILE_NAME);
#else
	cls = class_create(THIS_MODULE, DEVICE_FILE_NAME);
#endif

	/* /dev/char_devというデバイスノードを作成するs */
	device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);

	pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit chardev_exit(void) {
	/* /dev/char_devを削除 */
	device_destroy(cls, MKDEV(MAJOR_NUM, 0));

	/* 作成したデバイスクラスを削除 */
	class_destroy(cls);

	/* 登録したデバイスをカーネルから削除 */
	unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");