/**
 * @file reverse.h
 * 
 * 書き込まれた文字列を逆順にして返すキャラクタデバイス
 */
#ifndef REVERSE_H
#define REVERSE_H

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/types.h>

/**
 * @def デバイスの名前
 */
#define DEVICE_NAME "reverse"

/**
 * @def 書き込まれた内容を保持しておくためのバッファの最大サイズ
 */
#define BUF_LEN 80

/**
 * @def 成功フラグ. 関数が成功したときに返す
 */
#define SUCCESS 0

/**
 * @brief プロセスがデバイスファイルを開くときの処理
 */
static int device_open(struct inode *, struct file *);

/**
 * @brief プロセスがデバイスファイルを閉じるときの処理
 */
static int device_release(struct inode *, struct file *);

/**
 * @brief プロセスがデバイスファイルから読み込もうとしたときの処理
 */
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);

/**
 * @brief プロセスがデバイスファイルに書き込むときの処理
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

//! デバイスへの複数アクセスを防ぐための状態を持つ
extern atomic_t already_open;

//! デバイスが受け取るメッセージ
extern char msg[BUF_LEN + 1];

//! デバイスが反転させるメッセージ
extern char rev_msg[BUF_LEN + 1];

//! device_create()に使用するクラス構造体
extern struct class *cls;

//! コールバック関数を登録するためのfile_operations構造体
extern struct file_operations cdev_fops;

#endif /* REVERSE_H */