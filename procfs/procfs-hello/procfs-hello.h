/**
 * @file procfs-hello.h
 * 
 * cat /proc/helloworldにアクセスするとHello Worldを出力される
 */
#ifndef PROCFS_HELLO_H
#define PROCFS_HELLO_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/* カーネルのバージョンをチェックする */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

/**
 * @def /proc/に作成するファイルの名前
 */
#define procfs_name "helloworld"

/**
 * @brief /proc/helloworldを読み込んだときに呼び出される
 */
static ssize_t procfile_read(struct file *, char __user *, size_t, loff_t *);

#ifdef HAVE_PROC_OPS
extern const struct proc_ops proc_file_fops;
#else
extern const struct file_operations proc_file_fops;
#endif /* HAVE_PROC_OPS */

#endif /* PROCFS_HELLO_H */