/**
 * @file ioctl-2.h
 * 
 * ユーザ空間とカーネル空間の間で情報をやり取りするためのioctlインタフェースを定義する
 */
#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>	/* デバイスドライバとのやり取りを行うための_IO, _IOR, _IOW, _IOWRなどのマクロを提供する */

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
 * 
 * ユーザプログラムからカーネルモジュールにメッセージ(char *)を送る
 */
#define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *)

/**
 * @def 出力に使用され, デバイスドライバのメッセージを取得する
 * 
 * ユーザプログラムがカーネルからメッセージを取得する
 */
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)

/**
 * @def メッセージのnバイト目を取得する
 * 
 * ユーザプログラムからn(数値)をカーネルモジュールに渡し, カーネル側でmessage[n]の文字を取得して返す
 */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)

/**
 * @def デバイスファイル名
 */
#define DEVICE_FILE_NAME "char_dev"

/**
 * @def デバイスファイルのパス
 */
#define DEVICE_PATH "/dev/char_dev"

#endif /* CHARDEV_H */