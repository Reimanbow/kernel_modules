/**
 * @file sysfs-hello.c
 * 
 * sysfs経由でアクセス可能な変数の作成とhello world
 */
#include "sysfs-hello.h"

/**
 * kobjectはsysfsにエントリを作成するために使用されるカーネルオブジェクトである
 * my_moduleはsysfs内の/sys/kernel/my_moduleディレクトリを指す
 */
static struct kobject *my_module;

//! sysfsを通じてユーザ空間から読み書き可能な変数
int my_variable = 0;

/**
 * @brief my_variableの値を取得する関数
 * sysfsからmy_variableの値を読み出す
 * sprintfによりmy_variableの値を文字列に変換して, ユーザ空間に返す
 */
static ssize_t my_variable_show(struct kobject *kobj,
								struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", my_variable);
}

/**
 * @brief my_variableの値を変更する関数
 * sysfsから値を書き込むための関数
 * sscanfにより, ユーザ空間から受け取った文字列をintに変換しmy_variableに代入
 * countを返すことで, 書き込まれたバイト数をsysfsに通知する
 */
static ssize_t my_variable_store(struct kobject *kobj, struct kobj_attribute *attr,
								 const char *buf, size_t count)
{
	sscanf(buf, "%d", &my_variable);
	return count;
}

/**
 * @struct kobj_attribute
 * @brief sysfsの属性
 * __ATTR(name, permission, show, store)マクロを使ってsysfsの属性を定義する
 * name: sysfsの名前
 * permission: アクセス権限
 * show: 読み取り関数
 * store: 書き込み関数
 */
static struct kobj_attribute my_variable_attribute = 
	__ATTR(my_variable, 0660, my_variable_show, my_variable_store);

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init my_module_init(void) {
	int error = 0;

	pr_info("my_module: initialized\n");

	/* sysfs内に/sys/kernel/my_moduleディレクトリを作成 */
	my_module = kobject_create_and_add("my_module", kernel_kobj);
	if (!my_module) {
		return -ENOMEM;
	}

	/* /sys/kernel/my_module/my_variableファイルを作成 */
	error = sysfs_create_file(my_module, &my_variable_attribute.attr);
	/* ファイルの作成に失敗したらkobjectを解放する */
	if (error) {
		kobject_put(my_module);
		pr_info("failed to create the my_variable file in /sys/kernel/my_module\n");
	}

	return error;
}

static void __exit my_module_exit(void) {
	/* モジュール削除時にsysfsから/sys/kernel/my_moduleを削除 */
	pr_info("my_module: Exit success\n");
	kobject_put(my_module);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");