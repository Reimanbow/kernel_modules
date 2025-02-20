/**
 * @file chardev.h
 * 
 * devファイルから何回読み込んだかを示す, 読み取り専用のキャラクタデバイスを作成する
 */
#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/fs.h>		/* ファイルシステム関連の機能を提供する */
#include <linux/types.h>	/* カーネル内で使用されるデータ型を定義する */

/**
 * @def デバイスの名前
 */
#define DEVICE_NAME "chardev"

/**
 * @def バッファの最大サイズ
 */
#define BUF_LEN 80
/**
 * @def 成功フラグ. 関数が成功したときに返す
 */
#define SUCCESS 0

/**
 * @brief プロセスがsudo cat /dev/chardevのようにデバイスファイルを開こうとしたときに呼び出される
 */
static int device_open(struct inode *, struct file *);

/**
 * @brief デバイスファイルを閉じるときに呼び出される
 */
static int device_release(struct inode *, struct file *);

/**
 * @brief すでにdevファイルをオープンしているプロセスが, そのdevファイルから読み込もうとしたときに呼び出される
 */
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);

/**
 * @brief プロセスがdevファイルに書き込むときに呼び出される
 */
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

/**
 * @enum デバイスへの複数アクセスを防ぐための列挙体	
 */
enum {
	CDEV_NOT_USED,
	CDEV_EXCLUSIVE_OPEN,
};

//! デバイスドライバに割り当てられるメジャー番号
extern int major;

//! デバイスへの複数アクセスを防ぐために使用
extern atomic_t already_open;

//! デバイスが返すメッセージ
extern char msg[BUF_LEN + 1];

//! device_create()に使用するクラス構造体
extern struct class *cls;

//! コールバック関数を登録するためのfile_operations構造体の宣言
extern struct file_operations chardev_fops;

#endif /* CHARDEV_H */