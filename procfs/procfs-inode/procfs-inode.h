/**
 * @file procfs-inode.h
 * 
 * /proc/buffer2kというエントリを作成し, 書き込みと読み込みを行う
 * -- 書き込み: 最大2048バイトのprocfs_bufferにコピー
 * -- 読み込み: procfs_bufferの内容をユーザ空間にコピー. 一度読み込むと再度読み込めない
 */
#ifndef PROCFS_INODE_H
#define PROCFS_INODE_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include <linux/minmax.h>
#endif

/**
 * @def v5.6.0以降ならproc_ops構造体を使う
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

/**
 * @def バッファの最大サイズ
 */
#define PROCFS_MAX_SIZE 2048UL

/**
 * @def エントリの名前
 */
#define PROCFS_ENTRY_FILENAME "buffer2k"

//! 文字列を格納するバッファ
extern char procfs_buffer[PROCFS_MAX_SIZE];

//! バッファの現在のサイズ
extern unsigned long procfs_buffer_size;

#endif /* PROCFS_INODE_H */