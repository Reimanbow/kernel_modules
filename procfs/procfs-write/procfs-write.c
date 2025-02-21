/**
 * @file procfs-write.c
 * 
 * /procに書き込み可能なファイルを作成する
 */
#include "procfs-write.h"

//! procエントリの静的ポインタ
static struct proc_dir_entry *our_proc_file;

char procfs_buffer[PROCFS_MAX_SIZE];

unsigned long procfs_buffer_size = 0;

/**
 * @brief readが呼ばれたときの処理
 */
static ssize_t procfile_read(struct file *file, char __user *buffer,
							 size_t length, loff_t *offset)
{
	char s[13] = "HelloWorld!\n";
	int len = sizeof(s);
	ssize_t ret = len;

	if (*offset >= len || copy_to_user(buffer, s, len)) {
		pr_info("copy_to_user failed\n");
		ret = 0;
	} else {
		pr_info("procfile read %s\n", file->f_path.dentry->d_name.name);
		*offset += len;
	}

	return ret;
}

/**
 * @brief writeが呼ばれたときの処理
 */
static ssize_t procfile_write(struct file *file, const char __user *buffer,
							  size_t length, loff_t *offset)
{
	procfs_buffer_size = length;
	if (procfs_buffer_size >= PROCFS_MAX_SIZE) {
		procfs_buffer_size = PROCFS_MAX_SIZE - 1;
	}

	if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
		return -EFAULT;
	}

	procfs_buffer[procfs_buffer_size] = '\0';
	*offset += procfs_buffer_size;
	pr_info("procfile write %s\n", procfs_buffer);

	return procfs_buffer_size;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {
	.proc_read= procfile_read,
	.proc_write = procfile_write,
};
#else
static const struct file_operations proc_file_fops = {
	.read = procfile_read,
	.write = procfile_write,
};
#endif

static int __init procfs_init(void) {
	our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
	if (NULL == our_proc_file) {
		pr_alert("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
		return -ENOMEM;
	}

	pr_info("/proc/%s created\n", PROCFS_NAME);
	return 0;
}

static void __exit procfs_exit(void) {
	proc_remove(our_proc_file);
	pr_info("/proc/%s removed\n", PROCFS_NAME);
}

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");