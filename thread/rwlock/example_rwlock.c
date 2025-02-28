/**
 * @file example_rwlock.c
 * 
 * RWLockの使用方法を示すサンプル
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/rwlock.h>

/* 静的にRWロックの定義する */
static DEFINE_RWLOCK(myrwlock);

/**
 * @brief 読み込みロック
 */
static void example_read_lock(void) {
	unsigned long flags;

	/**
	 * 割り込みを禁止してロックを取得
	 * 複数のリーダが同時に取得可能
	 */
	read_lock_irqsave(&myrwlock, flags);
	pr_info("Read Locked\n");

	/* Read from something */

	/* ロックを解除し, 割り込みの状態を元に戻す */
	read_unlock_irqrestore(&myrwlock, flags);
	pr_info("Read Unlocked\n");
}

/**
 * @brief 書き込みロック
 */
static void example_write_lock(void) {
	unsigned long flags;

	/**
	 * 単独でロックを取得(他のスレッドをブロック)
	 * 読み取りスレッドもブロックされる
	 */
	write_lock_irqsave(&myrwlock, flags);
	pr_info("Write Locked\n");

	/* Write to something */

	/* ロックを解除し, 割り込みの状態を元に戻す */
	write_unlock_irqrestore(&myrwlock, flags);
	pr_info("Write Unlocked\n");
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init example_rwlock_init(void) {
	pr_info("example_rwlock started\n");

	example_read_lock();
	example_write_lock();

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit example_rwlock_exit(void) {
	pr_info("example_rwlock exit\n");
}

module_init(example_rwlock_init);
module_exit(example_rwlock_exit);

MODULE_DESCRIPTION("Read/Write locks example");
MODULE_LICENSE("GPL");