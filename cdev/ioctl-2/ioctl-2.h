/**
 * @file ioctl-2.h
 */
#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

/**
 * @def メジャー番号
 */
#define MAJOR_NUM 100

/**
 * @def ユーザ空間からカーネルモジュールに情報を渡すためのioctlコマンド番号を作成する
 * 
 * MAJOR_NUMは, 使用するメジャーデバイス番号である
 * 2番目の引数は, コマンドの番号である
 * 3番目の引数は, プロセスからカーネルに取得したい型である
 */
#define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *)

/**
 * @def 出力に使用され, デバイスドライバのメッセージを取得する
 */
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)

/**
 * @def メッセージのnバイト目を取得する
 * 
 * ユーザから数値nを受け取り, message[n]を返す
 */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)

/**
 * @def デバイスファイル名
 */
#define DEVICE_FILE_NAME "char_dev"
#define DEVICE_PATH "/dev/char_dev"

#endif /* CHARDEV_H */