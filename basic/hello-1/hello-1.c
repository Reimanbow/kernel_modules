/**
 * @file hello-1.c
 * 
 * 最もシンプルなカーネルモジュール
 */

#include <linux/module.h>   /* すべてのカーネルモジュールで必要になる */
#include <linux/printk.h>   /* pr_info()に必要である */

/* カーネルを組み込むときに呼ばれる関数 */
int init_module(void) {
    pr_info("Hello world 1.\n");

    /* 0以外を返すならinit_moduleは失敗 */
    return 0;
}

/* カーネルから削除される直前に呼ばれる */
void cleanup_module(void) {
    pr_info("Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");