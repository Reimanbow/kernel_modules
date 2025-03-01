/**
 * @file example_tasklet.c
 * 
 * タスクレットのサンプル
 */
#include <linux/delay.h>
#include <linux/interrupt.h>	/* タスクレットを使用するためのヘッダ */
#include <linux/module.h>
#include <linux/printk.h>

#ifndef DECLARE_TASKLET_OLD
#define DECLARE_TASKLET_OLD(arg1, arg2) DECLARE_TASKLET(arg1, arg2, 0L)
#endif

/**
 * @brief タスクレットが実行されると呼ばれる関数
 */
static void tasklet_fn(unsigned long data) {
	pr_info("Example tasklet starts\n");

	/* 5000ミリ秒のブロッキング遅延を発生 */
	mdelay(5000);
	
	pr_info("Example tasklet ends\n");
}

/* タスクレットの定義. tasklet_fnが処理関数として登録されている */
static DECLARE_TASKLET_OLD(mytask, tasklet_fn);

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init example_tasklet_init(void) {
	pr_info("tasklet example init\n");

	/**
	 * tasklet_schedule(&mytask);でタスクレットをスケジュール
	 * tasklet_fn()が後で実行される
	 */
	tasklet_schedule(&mytask);
	
	/* 200ミリ秒待機 */
	mdelay(200);
	pr_info("Example tasklet init continues...\n");
	return 0;
}

/**
 * @brief カーネルモジュールクリンナップ関数
 */
static void __exit example_tasklet_exit(void) {
	pr_info("tasklet example exit\n");
	
	/* taskletのスケジュールをキャンセル */
	tasklet_kill(&mytask);
}

module_init(example_tasklet_init);
module_exit(example_tasklet_exit);

MODULE_DESCRIPTION("Tasklet example");
MODULE_LICENSE("GPL");