/**
 * @file secret.h
 * 
 * write()で書き込んだデータを保存し, 1回だけread()で読み出せるデバイス
 */
#ifndef SECRET_H
#define SECRET_H

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/timekeeping.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include <linux/minmax.h>
#endif

/**
 * @def デバイスの名前
 */
#define DEVICE_NAME "secret"

/**
 * @def デバイスが保持できるデータの最大サイズ
 */
#define MAX_BUFFER_SIZE 80

/**
 * @enum デバイスへの複数アクセスを防ぐための列挙体
 */
enum {
	CDEV_NOT_USED,
	CDEV_EXCLUSIVE_OPEN,
};

//! 割り当てられるメジャー番号
extern int major;

//! データを受け取るバッファ
extern char dev_buffer[MAX_BUFFER_SIZE];

//! バッファのサイズ
extern size_t buffer_size;


#endif /* SECRET_H */