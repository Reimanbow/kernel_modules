/**
 * @file procfs-seq.h
 * 
 * seq_fileライブラリを使って, /procを操作する
 */
#ifndef PROCFS_SEQ_H
#define PROCFS_SEQ_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

/**
 * @def v5.6.0以降であれば, proc_ops構造体を使用する
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

/**
 * @def エントリの名前
 */
#define PROC_NAME "iter"

#endif /* PROCFS_SEQ_H */