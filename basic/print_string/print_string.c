/**
 * @file print_string.c
 * 
 * カーネルの現在のTTY(端末)に文字列を出力する
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>	/* get_current_tty()の仕様に必要 */
#include <linux/tty.h>		/* TTY関連の構造体・関数を提供 */

/**
 * @brief 現在のプロセスが属するTTYに文字列を出力する
 */
static void print_string(char *str) {
	/* 現在のプロセスが属するTTYに文字列を出力する */
	struct tty_struct *my_tty = get_current_tty();

	/* 取得に失敗するとNULLになるため何もしない */
	if (my_tty) {
		/* struct tty_operationsを取得する */
		const struct tty_operations *ttyops = my_tty->driver->ops;

		/* 文字列を出力する */
		(ttyops->write)(my_tty, str, strlen(str));

		/* 改行を挿入する */
		(ttyops->write)(my_tty, "\015\012", 2);
	}
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init print_string_init(void) {
	print_string("The module has been inserted. Hello world!");
	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit print_string_exit(void) {
	print_string("The module has been removed. Hello world!");
}

module_init(print_string_init);
module_exit(print_string_exit);

MODULE_LICENSE("GPL");