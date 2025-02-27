/**
 * @file completions.c
 * 
 * 1.モジュールをロードする
 * - completions_init()が実行され, スレッドを作成・起動
 * - "completions example"のログが出力される
 * 2.スレッド操作
 * - machine_crank_threadが"Turn the crank"を出力しcrank_compを完了
 * - machine_flywheel_spinup_threadがcrank_compを待機し, "Flywheel spins up"を出力
 * 3.モジュールをアンロード
 * - completions_exit()によりcrank_comp, flywheel_compが完了するのを待つ
 * - "completions exit"のログが出力される
 */
#include <linux/completion.h>	/* completion構造体と関連関数 */
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kthread.h>		/* カーネルスレッドの作成・管理用API */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/version.h>

/* クランク処理が完了したことを通知するcompletion変数 */
static struct completion crank_comp;
/* フライホイール処理が完了したことを通知するcompletion変数 */
static struct completion flywheel_comp;

/**
 * @brief クランクを回す動作をシミュレートする
 */
static int machine_crank_thread(void *arg) {
	pr_info("Turn the crank\n");

	/* crank_compを完了状態にする */
	complete_all(&crank_comp);

	/* スレッドを終了する */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	kthread_complete_and_exit(&crank_comp, 0);
#else
	complete_and_exit(&crank_comp, 0);
#endif
}

/**
 * @brief フライホイールを回す動作をシミュレートする
 */
static int machine_flywheel_spinup_thread(void *arg) {
	/* クランクが回るまで待機 */
	wait_for_completion(&crank_comp);

	pr_info("Flywheel spins up\n");

	/* flywheel_compを完了状態にする */
	complete_all(&flywheel_comp);
	
	/* スレッドを終了する */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	kthread_complete_and_exit(&flywheel_comp, 0);
#else
	complete_and_exit(&flywheel_comp, 0);
#endif
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init completions_init(void) {
	struct task_struct *crank_thread;
	struct task_struct *flywheel_thread;

	pr_info("completions example\n");

	/* completion変数を初期化 */
	init_completion(&crank_comp);
	init_completion(&flywheel_comp);

	/* クランクスレッドとフライホイールスレッドを作成 */
	crank_thread = kthread_create(machine_crank_thread, NULL, "KThread Crank");
	if (IS_ERR(crank_thread)) {
		goto ERROR_THREAD_1;
	}

	flywheel_thread = kthread_create(machine_flywheel_spinup_thread, NULL, "KThread Flywheel");
	if (IS_ERR(flywheel_thread)) {
		goto ERROR_THREAD_2;
	}

	/* スレッドを起動 */
	wake_up_process(flywheel_thread);
	wake_up_process(crank_thread);

	return 0;

ERROR_THREAD_2:
	kthread_stop(crank_thread);
ERROR_THREAD_1:
	return -1;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit completions_exit(void) {
	/* クランク処理が完了するまで待機 */
	wait_for_completion(&crank_comp);
	/* フライホイール処理が完了するまで待機 */
	wait_for_completion(&flywheel_comp);

	pr_info("completions exit\n");
}

module_init(completions_init);
module_exit(completions_exit);

MODULE_DESCRIPTION("Completions example");
MODULE_LICENSE("GPL");