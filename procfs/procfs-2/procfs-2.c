/**
 * @file procfs-2.c
 * 
 * ファイルの名前と,ファイルの一部の出力をコマンドラインから指定する
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

static char *file_name = "myinfo";
static char *file_value = "World";

module_param(file_name, charp, 0000);
MODULE_PARM_DESC(file_name, "file name");

module_param(file_value, charp, 0000);
MODULE_PARM_DESC(file_value, "file value");

/**
 * @brief /procにアクセスしたときに，文字列を出力する
 */
static int my_proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "Hello %s\n", file_value);
	return 0;
}

/**
 * @brief /procエントリが開かれたときにshow()関数を紐付ける
 */
static int my_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, my_proc_show, NULL);
}

/**
 * @brief /procの挙動を定義する
 */
static const struct proc_ops my_proc_ops = {
	.proc_open = my_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static struct proc_dir_entry *entry;

static int __init my_proc_init(void) {
	/**
	 * ファイルの名前はmyinfo
	 * 権限は0666
	 * 親ディレクトリはなし(/proc/myinfo)
	 * read()やwrite()などの処理を定義する構造体がmy_proc_ops
	 */
	entry = proc_create(file_name, 0666, NULL, &my_proc_ops);
	if (!entry) {
		return -ENOMEM;
	}
	pr_info("/proc/%s created\n", file_name);
	return 0;
}

static void __exit my_proc_exit(void) {
	/**
	 * /procのエントリを削除する
	 */
	proc_remove(entry);
	pr_info("/proc/%s removed\n", file_name);
}

module_init(my_proc_init);
module_exit(my_proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple /proc example");
MODULE_AUTHOR("Reimanbow");