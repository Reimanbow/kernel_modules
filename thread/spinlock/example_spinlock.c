/**
 * @file example_spinklock.c
 * 
 * 
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/spinlock.h>

/* 静的にスピンロックを初期化 */
static DEFINE_SPINLOCK(sl_static);
/* 動的にスピンロックを初期化(後でspin_lock_init()する) */
static spinlock_t sl_dynamic;

/**
 * @brief 静的に初期化したスピンロックの処理
 */
static void example_spinlock_static(void) {
	unsigned long flags;

	/**
	 * 割り込みを無効化しつつスピンロックを取得
	 * flagsに元の割り込み状態を保存
	 */
	spin_lock_irqsave(&sl_static, flags);

	/* クリティカルセクション */
	pr_info("Locked static spinlocks\n");

	/**
	 * ここで排他制御が行われる
	 */

	/* ロックを解除し, 割り込みを元の状態に復元 */
	spin_unlock_irqrestore(&sl_static, flags);
	pr_info("Unlocked static spinlock\n");
}

/**
 * @brief 動的に初期化したスピンロックの処理
 */
static void example_spinlock_dynamic(void) {
	unsigned long flags;

	/* 動的スピンロックの初期化 */
	spin_lock_init(&sl_dynamic);
	/* ロックを取得(割り込みを無効化) */
	spin_lock_irqsave(&sl_dynamic, flags);

	/* クリティカルセクション */
	pr_info("Locked dynamic spinlock\n");

	/**
	 * ここで排他制御が行われる
	 */

	/* ロックを解除し, 割り込みを元の状態に復元 */
	spin_unlock_irqrestore(&sl_dynamic, flags);
	pr_info("Unlocked dynamic spinlock\n");
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init example_spinlock_init(void) {
	pr_info("example spinlock started\n");

	example_spinlock_static();
	example_spinlock_dynamic();

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit example_spinlock_exit(void) {
	pr_info("example spinlock exit\n");
}

module_init(example_spinlock_init);
module_exit(example_spinlock_exit);

MODULE_DESCRIPTION("Spinlock example");
MODULE_LICENSE("GPL");