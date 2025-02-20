/**
 * @file hello-4.c
 * 
 * モジュールの情報について: License, Author, Description
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Reimanbow");
MODULE_DESCRIPTION("a simple driver");

static int __init init_hello_4(void) {
	pr_info("Hello world 4\n");
	return 0;
}

static void __exit cleanup_hello_4(void) {
	pr_info("Goodbye world 4\n");
}

module_init(init_hello_4);
module_exit(cleanup_hello_4);