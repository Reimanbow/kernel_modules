/**
 * @file mymem.c
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "mymem"
#define BUF_SIZE 1024

static char *kmalloc_buf;
static char *vmalloc_buf;
static struct proc_dir_entry *proc_entry;

/**
 * @brief デバイスファイルのread関数
 */
static ssize_t mymem_read(struct file *file, char __user *buf,
						  size_t count, loff_t *ppos)
{
	if (count > BUF_SIZE) count = BUF_SIZE;
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
	if (count > BUF_SIZE) count = BUF_SIZE;
	if (copy_from_user(kmalloc_buf, buf, count)) {
		return -EFAULT;
	}
	return count;
}

/**
 * @brief procfsの表示関数
 */
static int mymem_proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "kmalloc buffer address: %p\n", kmalloc_buf);
	seq_printf(m, "vmalloc buffer address: %p\n", vmalloc_buf);
	return 0;
}

static int mymem_proc_open(struct inode *inode, struct file *file) {
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

static int __init mymem_init(void) {
	int ret;

	kmalloc_buf = kmalloc(BUF_SIZE, GFP_KERNEL);
	/* kmallocでメモリ確保 */
	if (!kmalloc_buf) {
		pr_err("kmalloc failed");
		return -ENOMEM;
	}

	/* vmallocでメモリ確保 */
	vmalloc_buf = vmalloc(BUF_SIZE);
	if (!vmalloc_buf) {
		pr_err("vmalloc failed\n");
		kfree(kmalloc_buf);
		return -ENOMEM;
	}

	/* キャラクタデバイス登録 */
	ret = register_chrdev(0, DEVICE_NAME, &mymem_fops);
	if (ret < 0) {
		pr_err("Failed to register char device\n");
		vfree(vmalloc_buf);
		kfree(kmalloc_buf);
		return ret;
	}

	/* procfsに登録 */
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
	remove_proc_entry("mymem_info", NULL);
	unregister_chrdev(0, DEVICE_NAME);
	vfree(vmalloc_buf);
	kfree(kmalloc_buf);
	pr_info("mymem module unloaded\n");
}

module_init(mymem_init);
module_exit(mymem_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple memory allocation kernel module");