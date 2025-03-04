/**
 * @file mymem.c
 * 
 * kmalloc(物理メモリの連続領域確保)
 * vmalloc(仮想メモリの連続領域確保)
 */
#include <linux/module.h>		/* カーネルモジュール本体 */
#include <linux/fs.h>			/* ファイル操作(read/write) */
#include <linux/uaccess.h>		/* copy_from_user()などのユーザ空間データ操作 */
#include <linux/slab.h>			/* kmalloc()とkfree()用 */
#include <linux/vmalloc.h>		/* vmalloc()とvfree()用 */
#include <linux/proc_fs.h>		/* proc_fsでデータを保存するため */
#include <linux/seq_file.h>		/* proc_fsでデータを保存するため */

/* /dev/mymemという名前のデバイスを作成 */
#define DEVICE_NAME "mymem"
/* メモリ確保のサイズ(1KB) */
#define BUF_SIZE 1024

/* kmalloc()で確保したバッファ */
static char *kmalloc_buf;
/* vmalloc()で確保したバッファ */
static char *vmalloc_buf;
/* /proc/mymem_infoのprocfsエントリ */
static struct proc_dir_entry *proc_entry;

/**
 * @brief デバイスファイルのread関数
 */
static ssize_t mymem_read(struct file *file, char __user *buf,
						  size_t count, loff_t *ppos)
{
	/* ユーザが読み込みを要求したcountバイト分を制限(BUF_SIZEを超えないように) */
	if (count > BUF_SIZE) count = BUF_SIZE;
	/* カーネルメモリ(kmalloc_buf)の内容をユーザ空間にコピー */
	if (copy_to_user(buf, kmalloc_buf, count)) {
		return -EFAULT;
	}
	return count;
}

/**
 * @brief デバイスファイルのwrite関数
 */
static ssize_t mymem_write(struct file *file, const char __user *buf,
						   size_t count, loff_t *ppos)
{
	/* 書き込める最大バイト数をBUF_SIZEに制限 */
	if (count > BUF_SIZE) count = BUF_SIZE;
	/* copy_from_user()でユーザ空間からカーネルのkmalloc_bufにデータをコピー */
	if (copy_from_user(kmalloc_buf, buf, count)) {
		return -EFAULT;
	}
	return count;
}

/**
 * @brief procfsの表示関数
 */
static int mymem_proc_show(struct seq_file *m, void *v) {
	/* /proc/mymem_infoにkmalloc/vmallocのアドレスを表示 */
	seq_printf(m, "kmalloc buffer address: %p\n", kmalloc_buf);
	seq_printf(m, "vmalloc buffer address: %p\n", vmalloc_buf);
	return 0;
}

/**
 * @brief procfsエントリのopen関数
 */
static int mymem_proc_open(struct inode *inode, struct file *file) {
	/* procfsのデータ表示関数(mymem_proc_show()を登録) */
	return single_open(file, mymem_proc_show, NULL);
}

static const struct file_operations mymem_fops = {
	.owner = THIS_MODULE,
	.read = mymem_read,
	.write = mymem_write,
};

static const struct proc_ops mymem_proc_fops = {
	.proc_open = mymem_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/**
 * @brief カーネルモジュールの初期化
 */
static int __init mymem_init(void) {
	int ret;

	/* kmallocで1KBのメモリ確保 */
	kmalloc_buf = kmalloc(BUF_SIZE, GFP_KERNEL);
	if (!kmalloc_buf) {
		pr_err("kmalloc failed");
		return -ENOMEM;
	}

	/* vmallocで1KBのメモリ確保 */
	vmalloc_buf = vmalloc(BUF_SIZE);
	if (!vmalloc_buf) {
		pr_err("vmalloc failed\n");
		kfree(kmalloc_buf);
		return -ENOMEM;
	}

	/* /dev/mymemをキャラクタデバイス登録 */
	ret = register_chrdev(0, DEVICE_NAME, &mymem_fops);
	if (ret < 0) {
		pr_err("Failed to register char device\n");
		vfree(vmalloc_buf);
		kfree(kmalloc_buf);
		return ret;
	}

	/* /proc/mymem_infoを作成し, procfsに登録 */
	proc_entry = proc_create("mymem_info", 0, NULL, &mymem_proc_fops);
	if (!proc_entry) {
		unregister_chrdev(ret, DEVICE_NAME);
		vfree(vmalloc_buf);
		kfree(kmalloc_buf);
		return -ENOMEM;
	}

	pr_info("mymem module loaded\n");
	return 0;
}

/**
 * @brief カーネルモジュールの終了処理
 */
static void __exit mymem_exit(void) {
	/* /proc/mymem_infoを削除 */
	remove_proc_entry("mymem_info", NULL);
	/* /dev/mymemを削除 */
	unregister_chrdev(0, DEVICE_NAME);
	/* vfree()/kfree()で確保したメモリを解放 */
	vfree(vmalloc_buf);
	kfree(kmalloc_buf);
	pr_info("mymem module unloaded\n");
}

module_init(mymem_init);
module_exit(mymem_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple memory allocation kernel module");