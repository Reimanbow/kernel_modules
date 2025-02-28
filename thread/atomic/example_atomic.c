/**
 * @file example_atomic.o
 * 
 * atomic_tは整数操作をスレッドセーフにできる
 * bitops.hは, ビット単位でアトミック操作ができる
 */
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/printk.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte) \
	((byte & 0x80) ? '1' : '0'), ((byte & 0x40) ? '1' : '0'), \
	((byte & 0x20) ? '1' : '0'), ((byte & 0x10) ? '1' : '0'), \
	((byte & 0x08) ? '1' : '0'), ((byte & 0x04) ? '1' : '0'), \
	((byte & 0x02) ? '1' : '0'), ((byte & 0x01) ? '1' : '0')

/**
 * @brief アトミック整数操作
 * 
 * atomic_t型の変数 debbleとchrisを作成し, アトミック操作を実験する
 * atomic_tはスピンロック無しで安全にカウンタを変更できる
 */
static void atomic_add_subtract(void) {
	atomic_t debble;
	/* 初期値を50で初期化する */
	atomic_t chris = ATOMIC_INIT(50);

	/* debbleを45に設定 */
	atomic_set(&debble, 45);

	/* debbleを1減らす. debble=44 */
	atomic_dec(&debble);

	/* debbleに7を加算. debble=51 */
	atomic_add(7, &debble);

	/* debbleを1増やす. debble=52 */
	atomic_inc(&debble);

	/* debble=52とchris=50が出力される */
	pr_info("chris: %d, debble: %d\n", atomic_read(&chris), atomic_read(&debble));
}

/**
 * @brief アトミックビット操作
 * 
 * bitops.hを使ってアトミックナビット操作を実験する
 */
static void atomic_bitwise(void) {
	/* 初期状態 00000000 */
	unsigned long word = 0;
	pr_info("Bits 0: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));

	/* ビット3, 5をセット 00101000 */
	set_bit(3, &word);
	set_bit(5, &word);
	pr_info("Bits 1: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));

	/* ビット5をクリア 00001000 */
	clear_bit(5, &word);
	pr_info("Bits 2: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));
	
	/* ビット3をトグル 00000000 */
	change_bit(3, &word);
	pr_info("Bits 3: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));
	
	/**
	 * test_and_set_bit(x, y)
	 * - 引数に渡したときのyのxビット目の値を返す
	 * - その後, yのxビット目を1にする
	 * 00001000
	 */
	if (test_and_set_bit(3, &word)) {
		pr_info("wrong\n");
	}
	pr_info("Bits 4: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));

	/* word=11111111 */
	word = 255;
	pr_info("Bits 5: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(word));
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init example_atomic_init(void) {
	pr_info("example_atomic_started\n");

	atomic_add_subtract();
	atomic_bitwise();

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit example_atomic_exit(void) {
	pr_info("example_atomic exit\n");
}

module_init(example_atomic_init);
module_exit(example_atomic_exit);

MODULE_DESCRIPTION("Atomic operations example");
MODULE_LICENSE("GPL");