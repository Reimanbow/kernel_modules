/**
 * @file mutex_multithread.c
 * 
 * 2つのスレッドを作成し, ミューテックスの動作を試す
 * Thread1がロックを獲得し, 2病後に解放
 * Thread2はThread1が解放するまでロックを取得できない
 */
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>

/* ミューテックスの定義 */
static DEFINE_MUTEX(mymutex);
/* 2つのスレッドの変数を宣言する */
static struct task_struct *thread1, *thread2;

/**
 * @brief 2つのスレッドが行う処理
 * 
 * ロックを獲得し, 2秒間待機する. 待機後, ロックを解放する
 * 別のスレッドがロックを獲得していた場合は, 解放されるまで待機する
 */
static int thread_fn(void *arg) {
	pr_info("%s trying to lock mutex\n", (char *)arg);

	/* ロックを獲得する. すでに別のスレッドがロックを獲得していたら, 解放されるまで待つ */
	mutex_lock(&mymutex);
	pr_info("%s locked the mutex\n", (char *)arg);
	/* 2秒間ロックを保持 */
	msleep(2000);
	/* ロックを解除する */
	mutex_unlock(&mymutex);
	pr_info("%s unlocked the mutex\n", (char *)arg);

	return 0;
}

/**
 * @brief カーネルモジュール初期化関数
 * 
 * 2つのスレッドを作成し, 起動する
 */
static int __init example_mutex_init(void) {
	pr_info("example_mutex init\n");

	/* スレッドを作成し, 起動する */
	thread1 = kthread_run(thread_fn, "Thread 1", "mutex_thread1");
	thread2 = kthread_run(thread_fn, "Thread 2", "mutex_thread2");

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit example_mutex_exit(void) {
	pr_info("example_mutex exit\n");
}

module_init(example_mutex_init);
module_exit(example_mutex_exit);

MODULE_DESCRIPTION("Mutex example with threads");
MODULE_LICENSE("GPL");