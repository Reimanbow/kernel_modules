/**
 * @file userspace-ioctl.c
 * 
 * ioctlを使ってカーネルのデバイスドライバにデータを送受信することができる
 */
#include "ioctl-2.h"	/* ioctlにIOCTL_SET_MSGなどの#defineを取得 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/**
 * @brief メッセージをデバイスに送る
 */
int ioctl_set_msg(int file_desc, char *message) {
	int ret_val;

	/* IOCTL_SET_MSGを呼び出し, デバイスにメッセージを送る */
	ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

	if (ret_val < 0) {
		printf("ioctl_set_msg failed:%d\n", ret_val);
	}

	return ret_val;
}

/**
 * @brief デバイスからメッセージを取得
 */
int ioctl_get_msg(int file_desc) {
	int ret_val;
	char message[100] = {0};

	/* IOCTL_GET_MSGを呼び出し, デバイスからメッセージを取得 */
	ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

	if (ret_val < 0) {
		printf("ioctl_get_msg failed:%d\n", ret_val);
	}
	/* 取得したメッセージをprintfで表示 */
	printf("get_msg message:%s", message);

	return ret_val;
}

/**
 * @brief メッセージをiバイトずつ取得
 */
int ioctl_get_nth_byte(int file_desc) {
	int i, c;

	printf("get_nth_byte message:");

	i = 0;
	/* 1バイトずつデバイスから取得する */
	do {
		c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

		if (c < 0) {
			printf("\nioctl_get_nth_byte failed at the %d'th byte\n", i);
			return c;
		}

		/* 取得したバイトを表示 */
		putchar(c);
	} while (c != 0);

	return 0;
}

/**
 * @brief プログラムのエントリポイント
 */
int main(void) {
	/* デバイスファイルのファイルディスクリプタ */
	int file_desc, ret_val;
	/* デバイスに送るメッセージ */
	char *msg = "Message passed by ioctl\n";

	/* デバイスファイルを開く */
	file_desc = open(DEVICE_PATH, O_RDWR);
	if (file_desc < 0) {
		printf("Can't open device file: %s, error: %d\n", DEVICE_PATH, file_desc);
		exit(EXIT_FAILURE);
	}

	/* メッセージを書き込む */
	ret_val = ioctl_set_msg(file_desc, msg);
	if (ret_val) {
		goto error;
	}

	/* 1バイトずつ取得する */
	ret_val = ioctl_get_nth_byte(file_desc);
	if (ret_val) {
		goto error;
	}

	/* メッセージ全体を取得 */
	ret_val = ioctl_get_msg(file_desc);
	if (ret_val) {
		goto error;
	}

	close(file_desc);
	return 0;
error:
	close(file_desc);
	exit(EXIT_FAILURE);
}