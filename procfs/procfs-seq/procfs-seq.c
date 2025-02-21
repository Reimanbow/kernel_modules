/**
 * @file procfs-seq.h
 * 
 * seq_fileライブラリを使って, /procを操作する
 */
#include "procfs-seq.h"

/**
 * @brief シーケンスの最初に呼ばれる関数. /procファイルが読み込まれるとき. または関数停止後（シーケンス終了後）
 */
static void *my_seq_start(struct seq_file *s, loff_t *pos) {
	static unsigned long counter = 0;

	/* 新しいシーケンスかどうか */
	if (*pos == 0) {
		/* Yes: シーケンスを開始するNULLでない値を返す */
		return &counter;
	}

	/* No: シーケンスの終わりである */
	*pos = 0;
	return NULL;
}

/**
 * @brief シーケンスの開始後に呼び出される. 戻り値がNULLになるまで呼び出される
 */
static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos) {
	unsigned long *tmp_v = (unsigned long *)v;
	(*tmp_v)++;
	(*pos)++;
	return NULL;
}

/**
 * @brief シーケンスの終わりに呼び出される
 */
static void my_seq_stop(struct seq_file *s, void *v) {
	/* 何もしない */
}

/**
 * @brief シーケンスのステップごとに呼び出される. nextが呼び出されるごとに呼び出される
 */
static int my_seq_show(struct seq_file *s, void *v) {
	loff_t *spos = (loff_t *)v;

	seq_printf(s, "%Ld\n", *spos);
	return 0;
}

/**
 * @struct seq_operations
 * @brief シーケンスを管理するための関数を集めている構造体
 */
static struct seq_operations my_seq_ops = {
	.start = my_seq_start,
	.next = my_seq_next,
	.stop = my_seq_stop,
	.show = my_seq_show,
};

/**
 * @brief open()が呼び出されたときの処理
 */
static int my_open(struct inode *inode, struct file *file) {
	return seq_open(file, &my_seq_ops);
}

/**
 * v5.6.0以降ならproc_opsを使用する
 */
#ifdef HAVE_PROC_OPS
static const struct proc_ops my_file_ops = {
	.proc_open = my_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};
#else
static const struct file_operations my_file_ops = {
	.open = my_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
}
#endif

/**
 * @brief カーネル初期化関数
 */
static int __init procfs_init(void) {
	struct proc_dir_entry *entry;

	entry = proc_create(PROC_NAME, 0, NULL, &my_file_ops);
	if (entry == NULL) {
		pr_debug("Error: Could not initialize /proc/%s\n", PROC_NAME);
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief カーネルクリーンアップ関数
 */
static void __exit procfs_exit(void) {
	remove_proc_entry(PROC_NAME, NULL);
	pr_debug("/proc/%s removed\n", PROC_NAME);
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");