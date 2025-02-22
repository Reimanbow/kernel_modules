/**
 * @file cdev-time.h
 * 
 * cat /dev/time を実行すると, 現在の日時を取得できるデバイス
 */
#ifndef CDEV_TIME_H
#define CDEV_TIME_H

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

/**
 * @def デバイスの名前
 */
#define DEVICE_NAME "time"

/**
 * @enum デバイスへの複数アクセスを防ぐための列挙体
 */
enum {
	CDEV_NOT_USED,
	CDEV_EXCLUSIVE_OPEN,
};

//! 割り当てられるメジャー番号
extern int major;

#endif /* CDEV_TIME_H */