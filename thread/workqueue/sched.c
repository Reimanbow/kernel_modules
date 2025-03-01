/**
 * @file sched.c
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>	/* workqueueを使用するための機能を提供する */

/* workqueueの本体 */
static struct workqueue_struct *queue = NULL;
/* workqueueに追加される作業 */
static struct work_struct work;

/**
 * @brief Workqueueに登録する関数
 */
static void work_handler(struct work_struct *data) {
	pr_info("work handler function.\n");
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init sched_init(void) {
	/**
	 * workqueueを作成
	 * WQ_UNBOUND: CPUの制約を受けずに任意のCPUで実行
	 * 1: 最大1つのジョブを同時に処理
	 */
	queue = alloc_workqueue("HELLOWORLD", WQ_UNBOUND, 1);
	
	/* workにwork_handler()を登録 */
	INIT_WORK(&work, work_handler);

	/* workqueueにっジョブを追加し, 非同期で実行される */
	queue_work(queue, &work);
	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit sched_exit(void) {
	/* workqueueを削除する */
	destroy_workqueue(queue);
}

module_init(sched_init);
module_exit(sched_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Workqueue example");