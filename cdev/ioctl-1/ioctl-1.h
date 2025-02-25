/**
 * @file ioctl.h
 * 
 * ユーザプログラムとカーネルの間で, 入出力制御を使用するキャラクタデバイスドライバ
 * ユーザプログラムがこのドライバを通じて特定のデバイスにデータを読み書きできる
 */
#ifndef IOCTL_H
#define IOCTL_H

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/**
 * @struct ioctl_arg
 * @brief ユーザ空間とカーネル空間の間でデータをやり取りするための構造体
 */
struct ioctl_arg {
	unsigned int val;
};

/**
 * @def ioctlコマンドを識別するためのマジックナンバー
 * \x66は, 他のデバイスとコマンドが衝突しないように設定する
 */
#define IOC_MAGIC '\x66'

/**
 * ioctlコマンドの定義
 * _IOW(マジックナンバー, コマンド番号, データ型): ユーザ空間->カーネル空間へデータを送る
 * _IOR(マジックナンバー, コマンド番号, データ型): カーネル空間->ユーザ空間へデータを送る
 */
#define IOCTL_VALSET _IOW(IOC_MAGIC, 0, struct ioctl_arg)

#define IOCTL_VALGET _IOR(IOC_MAGIC, 1, struct ioctl_arg)

#define IOCTL_VALGET_NUM _IOR(IOC_MAGIC, 2, int)

#define IOCTL_VALSET_NUM _IOW(IOC_MAGIC, 3, int)

/**
 * @def ioctlの最大コマンド番号を定義(0から3まで)
 */
#define IOCTL_VAL_MAXNR 3

/**
 * @def /dev/ioctltestのようにデバイスファイルの名前
 */
#define DRIVER_NAME "ioctltest"

//! デバイスのメジャー番号
extern unsigned int test_ioctl_major;

//! 作成するデバイスの数
extern unsigned int num_of_dev;

//! 数値データのやり取りに使う変数
extern int ioctl_num;

/**
 * @struct test_ioctl_data
 * @brief デバイスの内部データ. valという変数と, 排他制御用のrwlock_t lockを持つ
 */
struct test_ioctl_data {
	//! デバイスの状態を保持する変数(ioctlやreadでアクセス可能)
	unsigned char val;
	//! データ競合を防ぐための排他制御ロック
	rwlock_t lock;
};

#endif /* IOCTL_H */