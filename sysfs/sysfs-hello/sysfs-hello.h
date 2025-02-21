/**
 * @file sysfs-hello.h
 * 
 * sysfs経由でアクセス可能な変数の作成とhello world
 */
#ifndef SYSFS_HELLO_H
#define SYSFS_HELLO_H

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kobject.h>	/* sysfsで利用するkobject（カーネルオブジェクト）関連の定義を含む */
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>	/* sysfsにファイルを作成するための関数を含む */

extern int my_variable;

#endif