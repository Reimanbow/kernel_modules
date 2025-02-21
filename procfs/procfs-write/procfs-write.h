/**
 * @file procfs-write.h
 * 
 * /procに書き込み可能なファイルを作成する
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/* v5.6.0ならproc_ops構造体を使う */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

/**
 * @def 文字列の最大サイズ
 */
#define PROCFS_MAX_SIZE 1024

/**
 * @def 作成するファイルの名前
 */
#define PROCFS_NAME "buffer1k"

//! モジュールの文字格納用バッファ
extern char procfs_buffer[PROCFS_MAX_SIZE];

//! バッファのサイズ
extern unsigned long procfs_buffer_size;
