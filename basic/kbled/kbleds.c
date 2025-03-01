/**
 * @file kbleds.c
 * 
 * モジュールがアンロードされるまでキーボードが点滅する
 */
#include <linux/init.h>
#include <linux/kd.h>
#include <linux/module.h>
#include <linux/tty.h>				/* TTY(端末)関連の構造体・関数を提供 */
#include <linux/vt.h>				/* 仮想端末(VT)の制御 */
#include <linux/vt_kern.h>			/* カーネル内で仮想端末を操作するための関数を提供 */
#include <linux/console_struct.h>	/* コンソールデバイスの管理構造体を提供 */

MODULE_DESCRIPTION("Example module illustrating the use of Keyboard LEDs");

/* LEDの点滅を制御するためのtimer_list構造体 */
static struct timer_list my_timer;
/* TTYドライバへのポインタ */
static struct tty_driver *my_driver;
/* 現在のLED状態(点灯・消灯) */
static unsigned long kbledstatus = 0;

/* LEDの点滅間隔を設定(HZ/5=1秒の1/5=約200ms) */
#define BLINK_DELAY HZ / 5
/* Num Lock, Caps Lock, Scroll LockのすべてをON */
#define ALL_LEDS_ON 0x07
/* 元のLED状態に戻す */
#define RESTORE_LEDS 0xFF

/**
 * @brief タイマーコールバック関数
 */
static void my_timer_func(struct timer_list *unused) {
	/* 現在のフォアグランドコンソールのTTY構造体を取得 */
	struct tty_struct *t = vc_cons[fg_console].d->port.tty;

	/* kblestatusをALL_LED_ONとRESTORE_LEDSで切り替え */
	if (kbledstatus == ALL_LEDS_ON) {
		kbledstatus = RESTORE_LEDS;
	} else {
		kbledstatus = ALL_LEDS_ON;
	}

	/* KDSETLEDをioctlで実行し, LEDの状態を変更 */
	(my_driver->ops->ioctl)(t, KDSETLED, kbledstatus);

	/* jiffies(カーネルの時間管理用変数)を使って次のタイマーイベントを設定 */
	my_timer.expires = jiffies + BLINK_DELAY;
	
	/* タイマーを再登録して, 再び関数を実行する */
	add_timer(&my_timer);
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init kbleds_init(void) {
	int i;

	pr_info("kbleds: loading\n");
	/* 現在アクティブなコンソールのインデックス */
	pr_info("kbleds: fgconsole is %x\n", fg_console);
	
	/* 各仮想コンソールの番号を取得し, ログに出力 */
	for (i = 0; i < MAX_NR_CONSOLES; i++) {
		if (! vc_cons[i].d) {
			break;
		}
		pr_info("poet_atkm: console[%i/%i] #%i, tty %p\n", i, MAX_NR_CONSOLES,
				vc_cons[i].d->vc_num, (void *)vc_cons[i].d->port.tty);
	}
	pr_info("kbleds: finished scanning consoles\n");

	/* フォアグランドのコンソールのTTYドライバを取得 */
	my_driver = vc_cons[fg_console].d->port.tty->driver;
	pr_info("kbleds: tty driver name %sn", my_driver->driver_name);

	/* my_timer_funcをタイマーに設定 */	
	timer_setup(&my_timer, my_timer_func, 0);

	/* 初回のタイマーイベントをスケジュール */
	my_timer.expires = jiffies + BLINK_DELAY;

	/* タイマーを開始 */
	add_timer(&my_timer);

	return 0;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit kbleds_cleanup(void) {
	pr_info("kbleds: unloading...\n");

	/* タイマーを削除(点滅を停止) */
	del_timer(&my_timer);

	/* KDSETLEDをRESTORE_LEDSに設定し, LEDを元の状態に戻す */
	(my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);

MODULE_LICENSE("GPL");