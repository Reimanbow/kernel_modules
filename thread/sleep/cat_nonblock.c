/**
 * @file cat_nonblock.c
 * 
 * ファイルの内容を表示するが, 非ブロッキング(O_NONBLOCK)モードでファイルを開く
 * カーネルモジュールが提供するブロッキング可能なファイルに対して動作確認する
 * 
 * 引数で指定されたファイルを開く(O_NONBLOCKを指定)
 * ブロックすることなくデータを読み取り, 標準出力に表示
 * データがない場合, 即座に終了する(catのように待機しない)
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_BYTES 1024 * 4

int main(int argc, char *argv[]) {
	int fd;
	size_t bytes;
	char buffer[MAX_BYTES];

	/* 引数が1つでない場合, エラーメッセージを表示して終了する */
	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		puts("Reads the content of a file, but doesn't wait for input");
		exit(EXIT_FAILURE);
	}

	/**
	 * 非ブロッキングでファイルを開く
	 * 通常, プロセスはopen()を呼び出したときにファイルが使用中ならブロックされる
	 * O_NONBLOCKを指定すると, ファイルが開けない場合でも待たずにエラーを返す
	 */
	fd = open(argv[1], O_RDONLY | O_NONBLOCK);

	/* open()のエラーチェック */
	if (fd == -1) {
		/**
		 * EAGAINなら, 開こうとしたがブロックするので, 開くのを諦めた
		 * その他のエラーなら, 開くのに失敗した
		 */
		puts(errno == EAGAIN ? "Open would block" : "Open failed");
		exit(EXIT_FAILURE);
	}

	/* データを読み取る */
	do {
		/* ファイルから4096バイト読み取る */
		bytes = read(fd, buffer, MAX_BYTES);

		/* read()のエラーチェック */
		if (bytes == -1) {
			/* EAGAINの場合は, 通常はブロックするが, O_NONBLOCKなのでしないことを示すメッセージを出力 */
			if (errno == EAGAIN) {
				puts("Normally I'd block, but you told me not to");
			} else {
				puts("Another read error");
				exit(EXIT_FAILURE);
			}
		}

		/* 読み取ったデータを標準出力に表示 */
		if (bytes > 0) {
			for (int i = 0; i < bytes; i++) {
				putchar(buffer[i]);
			}
		}
	} while (bytes > 0);

	close(fd);
	return 0;
}