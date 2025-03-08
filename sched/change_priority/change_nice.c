/**
 * @file change_nice.c
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/printk.h>

/* PID */
static int pid = -1;
/* nice値(-20~19) */
static int nice_value = 0;

module_param(pid, int , 0);
module_param(nice_value, int, 0);

static void change_nice_value(int pid, int nice_value) {
	struct task_struct *task;

	if (pid == -1) {
		pr_info("Invalid PID\n");
		return;
	}

	/* nice値の範囲チェック(-20~19) */
	if (nice_value < -20 || nice_value > 19) {
		pr_info("Invalid nice value: %d (must be between -20 and 19)\n", nice_value);
		return;
	}

	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (!task) {
		pr_info("Process with PID %d not found\n", pid);
		return;
	}

	set_user_nice(task, nice_value);

	pr_info("Changed PID %d nice value to %d\n", pid, nice_value);
}

static int __init my_module_init(void) {
	pr_info("Change nice value for PID: %d\n", pid);
	change_nice_value(pid, nice_value);
	return 0;
}

static void __exit my_module_exit(void) {
	pr_info("Nice value changer module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");