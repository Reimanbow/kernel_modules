/**
 * @file procfs-hello.c
 * 
 * cat /proc/helloworldにアクセスするとHello Worldを出力される
 */
#include "procfs-hello.h"

//! procエントリの静的ポインタ
static struct proc_dir_entry *our_proc_file;

/**
 * @brief /proc/helloworldを読み込んだときに呼び出される
 */
static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
							 size_t buffer_length, loff_t *offset)
{
	char s[13] = "HelloWorld!\n";
	int len = sizeof(s);
	ssize_t ret = len;

	/* ユーザ空間にコピーする=cat /proc/helloworldを実行したら, 渡した文字列が出力される */
	if (*offset >= len || copy_to_user(buffer, s, len)) {
		pr_info("copy_to_user failed\n");
		ret = 0;
	} else {
		pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
		*offset += len;
	}

	return ret;
}

#ifdef HAVE_PROC_OPS
const struct proc_ops proc_file_fops = {
	.proc_read = procfile_read,
};
#else
const struct file_operations proc_file_fops = {
	.read = procfle_read,
};
#endif /* HAVE_PROC_OPS */

static int __init procfs_init(void) {
	/* /proc/helloworldを出力する */
	our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
	if (NULL == our_proc_file) {
		pr_alert("Error: Could not initialize /proc/%s\n", procfs_name);
		return -ENOMEM;
	}

	pr_info("/proc/%s created\n", procfs_name);
	return 0;
}

static void __exit procfs_exit(void) {
	proc_remove(our_proc_file);
	pr_info("/proc/%s removed\n", procfs_name);
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");