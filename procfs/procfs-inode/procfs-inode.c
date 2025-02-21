/**
 * @file procfs-inode.c
 * 
 * /proc/buffer2kというエントリを作成し, 書き込みと読み込みを行う
 * -- 書き込み: 最大2048バイトのprocfs_bufferにコピー
 * -- 読み込み: procfs_bufferの内容をユーザ空間にコピー. 一度読み込むと再度読み込めない
 */
#include "procfs-inode.h"

//! procエントリの静的ポインタ
static struct proc_dir_entry *our_proc_file;

//! 文字列を格納するバッファ
char procfs_buffer[PROCFS_MAX_SIZE];

//! バッファの現在のサイズ
unsigned long procfs_buffer_size = 0;

/**
 * @brief readが呼び出されたときの処理
 */
static ssize_t procfs_read(struct file *file, char __user *buffer,
						   size_t length, loff_t *offset)
{
	/* /procに何も書き込まれていなければ, 出力するものがないため失敗する */
	if (*offset || procfs_buffer_size == 0) {
		pr_debug("procfs_read: END\n");
		*offset = 0;
		return 0;
	}

	/* カーネル空間に保持していたバッファをユーザ空間に出力する */
	procfs_buffer_size = min(procfs_buffer_size, length);
	if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size)) {
		return -EFAULT;
	}
	*offset += procfs_buffer_size;

	pr_debug("procfs_read: read %lu bytes\n", procfs_buffer_size);
	return procfs_buffer_size;
}

/**
 * @brief write()が呼び出されたときの処理
 */
static ssize_t procfs_write(struct file *file, const char __user *buffer,
							size_t length, loff_t *offset)
{
	/* 書き込まれた文字列のサイズを取得する */
	procfs_buffer_size = min(PROCFS_MAX_SIZE, length);

	/* 書き込まれた文字列をカーネル空間にコピーする */
	if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
		return -EFAULT;
	}
	*offset += procfs_buffer_size;

	pr_debug("procfs_write: write %lu bytes\n", procfs_buffer_size);
	return procfs_buffer_size;
}

/**
 * @brief open()が呼び出されたときの処理
 */
static int procfs_open(struct inode *inode, struct file *file) {
	/* モジュールの参照カウントを増やし, アンロードされるのを防ぐ */
	try_module_get(THIS_MODULE);
	return 0;
}

/**
 * @brief close()が呼び出されたときの処理
 */
static int procfs_close(struct inode *inode, struct file *file) {
	/* モジュールの参照カウントを減らす */
	module_put(THIS_MODULE);
	return 0;
}

/* v5.6.0以降ならproc_ops構造体を使う */
#ifdef HAVE_PROC_OPS
static struct proc_ops file_ops_4_our_proc_file = {
	.proc_read = procfs_read,
	.proc_write = procfs_write,
	.proc_open = procfs_open,
	.proc_release = procfs_close,
};
#else
static struct file_operations file_ops_4_our_proc_file = {
	.read = procfs_read,
	.write = procfs_write,
	.open = procfs_open,
	.release = procfs_close,
};
#endif

/**
 * @brief カーネル初期化関数
 */
static int __init procfs_init(void) {
	/* /procにエントリを作成する */
	our_proc_file = proc_create(PROCFS_ENTRY_FILENAME, 0644, NULL,
								&file_ops_4_our_proc_file);
	if (our_proc_file == NULL) {
		pr_debug("Error: Could not initialize /proc/%s\n",
				 PROCFS_ENTRY_FILENAME);
		return -ENOMEM;
	}

	/* ファイルのサイズを80に指定する */
	proc_set_size(our_proc_file, 80);
	/* 所有者をroot, 所有グループをrootにしている */
	proc_set_user(our_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);

	pr_debug("/proc/%s created\n", PROCFS_ENTRY_FILENAME);
	return 0;
}

/**
 * @brief カーネルクリーンアップ関数
 */
static void __exit procfs_exit(void) {
	/* /procからエントリを削除する */
	remove_proc_entry(PROCFS_ENTRY_FILENAME, NULL);
	pr_debug("/proc/%s removed\n", PROCFS_ENTRY_FILENAME);
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");