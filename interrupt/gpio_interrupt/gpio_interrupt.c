#include <linux/gpio/consumer.h>	/* gpiod API */
#include <linux/gpio/driver.h>		/* GPIOドライバのサポート */
#include <linux/interrupt.h>		/* 割り込み処理 */
#include <linux/kernel.h>			/* カーネルの基本機能 */
#include <linux/module.h>			/* モジュール管理 */
#include <linux/of.h>				/* デバイスツリー管理 */
#include <linux/platform_device.h>	/* プラットフォームデバイス管理 */

/* gpio_desc *を使用してGPIOを管理(libgpiodの方式) */
static struct gpio_desc *led;
static struct gpio_desc *button_on;
static struct gpio_desc *button_off;
/* button_irqs[]にボタンの割り込み番号を格納 */
static int button_irqs[2] = { -1, -1 };

/**
 * @brief 割り込みハンドラ
 */
static irqreturn_t button_isr(int irq, void *data) {
	if (irq == button_irqs[0] && gpiod_get_value(led) == 0) {
		gpiod_set_value(led, 1);
	} else if (irq == button_irqs[1] && gpiod_get_value(led) == 1) {
		gpiod_set_value(led, 0);
	}
	return IRQ_HANDLED;
}

/**
 * @brief 初期化関数
 */
static int __init intrpt_init(void) {
	/* デバイスツリーからGPIOを取得するための変数 */
	struct device *dev = NULL;
	int ret;

	pr_info("intrpt_init\n");

	/**
	 * GPIOの取得
	 * デバイスツリーからGPIOを取得し, 初期状態をLOWに設定
	 */
	led = gpiod_get(dev, "led", GPIOD_OUT_LOW);
	if (IS_ERR(led)) {
		pr_err("Failed to request LED GPIO\n");
		return PTR_ERR(led);
	}

	/* ボタンON用GPIOを取得(入力モード) */
	button_on = gpiod_get(dev, "button_on", GPIOD_IN);
	if (IS_ERR(button_on)) {
		pr_err("Failed to request BUTTON ON GPIO\n");
		gpiod_put(led);
		return PTR_ERR(button_on);
	}

	/* ボタンOFF用GPIOも同様に取得 */
	button_off = gpiod_get(dev, "button_off", GPIOD_IN);
	if (IS_ERR(button_off)) {
		pr_err("Failed to request BUTTON OFF GPIO\n");
		gpiod_put(button_on);
		gpiod_put(led);
		return PTR_ERR(button_off);
	}

	/* GPIOから割り込み番号を取得(ボタンON) */
	button_irqs[0] = gpiod_to_irq(button_on);
	if (button_irqs[0] < 0) {
		pr_err("Failed to get IRQ for BUTTON ON\n");
		goto fail;
	}

	/* GPIOから割り込み番号を取得(ボタンOFF) */
	button_irqs[1] = gpiod_to_irq(button_off);
	if (button_irqs[1] < 0) {
		pr_err("Failed to get IRQ for BUTTON OFF\n");
		goto fail;
	}

	/* ボタンONの割り込みを登録(立ち上がり・立ち下がりで発生) */
	ret = request_irq(button_irqs[0], button_isr,
					  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					  "button_on_irq", NULL);
	if (ret) {
		pr_err("Failed to request IRQ for BUTTON ON\n");
		goto fail;
	}

	/* ボタンONの割り込みを登録(立ち上がり・立ち下がりで発生) */
	ret = request_irq(button_irqs[1], button_isr,
					  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					  "button_off_irq", NULL);
	if (ret) {
		pr_err("Failed to request IRQ for BUTTON OFF\n");
		free_irq(button_irqs[0], NULL);
		goto fail;
	}

	return 0;

	/* 失敗処理 */
fail:
	gpiod_put(button_on);
	gpiod_put(button_off);
	gpiod_put(led);
	return -1;
}

/**
 * @brief モジュールの解放処理
 */
static void __exit intrpt_exit(void) {
	pr_info("intrpt_exit\n");
	free_irq(button_irqs[0], NULL);
	free_irq(button_irqs[1], NULL);
	gpiod_set_value(led, 0);
	gpiod_put(led);
	gpiod_put(button_on);
	gpiod_put(button_off);
}

module_init(intrpt_init);
module_exit(intrpt_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO Interrupt Example using gpiod");