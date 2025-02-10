/** 
 * @file hello.c
 * 
 * Linuxカーネルモジュール作ってみた
 * 参考->https://qiita.com/nouernet/items/77bdea90e5423d153c8d
 */

#include <linux/module.h> // すべてのモジュールに必要なヘッダファイル
#include <linux/kernel.h> // KERN_INFOに必要

int init_module(void) {
    printk(KERN_INFO "Hello world 1.\n");
    return 0;
}

void cleanup_module(void) {
    printk(KERN_INFO "Goodbye world 1.\n");
}

MODULE_LICENSE("GPL v2");