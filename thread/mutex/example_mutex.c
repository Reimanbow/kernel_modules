/**
 * @file example_mutex.c
 * 
 * Linuxカーネルのミューテックスを使用する
 */
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>

/**
 * ミューテックスの定義
 * - DEFINE_MUTEX(mymutex);は静的にミューテックスを初期化するマクロ
 * - mymutexという名前のミューテックスを作成
 */
static DEFINE_MUTEX(mymutex);

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init example_mutex_init(void) {
	int ret;

	pr_info("example_mutex init\n");

	/**
	 * mutex_trylock(&mymutex)でミューテックスを取得
	 * - 非ブロッキングでミューテックスをロックする関数
	 *   - 成功すれば1を返す
	 *   - 失敗すれば0を返す(他のプロセスがすでにロックしている場合)
	 */
	ret = mutex_trylock(&mymutex);
	/* ロック成功の場合の処理 */
	if (ret != 0) {
		pr_info("mutex is locked\n");

		/* ミューテックスを解放 */
		mutex_unlock(&mymutex);
		pr_info("mutex is unlocked\n");
	/* ロックできなかった場合の処理 */
	} else {
		pr_info("Failed to lock\n");
	}

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

MODULE_DESCRIPTION("Mutex example");
MODULE_LICENSE("GPL");