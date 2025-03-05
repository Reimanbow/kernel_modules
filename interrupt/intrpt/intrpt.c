/**
 * @file intrpt.c
 */
#include <linux/gpio.h>			/* GPIO制御のための関数やデータ構造を提供 */
#include <linux/interrupt.h>	/* IRQ(割り込み)処理用 */
#include <linux/kernel.h>		/* カーネル汎用関数のため */
#include <linux/module.h>		/* カーネルモジュールの処理 */
#include <linux/printk.h>		/* ログ出力用 */
#include <linux/version.h>		/* カーネルのバージョンチェック */

/** 
 * カーネルバージョンが6.10.0以上なら, 
 * gpio_request_array()の代わりに, gpio_request()を使用する
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
#define NO_GPIO_REQUEST_ARRAY
#endif

/**
 * ボタン用のIRQ番号を格納する配列
 * 初期値-1は未定義を意味する
 */
static int button_irqs[] = { -1, -1 };

/* GPIO 4に接続されたLED(初期状態はLOW=消灯) */
static struct gpio leds[] = { { 4, GPIOF_OUT_INIT_LOW, "LED 1" } };

/* GPIO 17, 18に接続されたボタン(入力用) */
static struct gpio buttons[] = { { 17, GPIOF_IN, "LED 1 ON BUTTON" },
								 { 18, GPIOF_IN, "LED 1 OFF BUTTON" } };

/**
 * @brief 割り込みハンドラ
 * 
 * @param irq 発生したIRQの番号
 * @param data 通常NULL
 * 
 * @return IRQ_HANDLED
 * 
 * 動作
 * 1.button_irqs[0](ボタン1)が押され, LEDがOFFの場合, LEDをONにする
 * 2.button_irqs[1](ボタン2)が押され, LEDがONの場合, LEDをOFFにする
 */
static irqreturn_t button_isr(int irq, void *data) {
	if (irq == button_irqs[0] && !gpio_get_value(leds[0].gpio)) {
		gpio_set_value(leds[0].gpio, 1);
	} else if (irq == button_irqs[1] && gpio_get_value(leds[0].gpio)) {
		gpio_set_value(leds[0].gpio, 0);
	}

	return IRQ_HANDLED;
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init intrpt_init(void) {
	int ret = 0;

	/* 現在の関数名(intrpt_init)をカーネルログに出力する */
	pr_info("%s\n", __func__);

	/**
	 * LED GPIOのリクエスト
	 * - gpio_request(): 指定したGPIOをカーネルにリクエスト(単一)
	 * - gpio_request_array(): 複数のGPIOをまとめてリクエスト(古いカーネル向け)
	 * - エラー時にはretに負の値がセットされる
	 */
#ifdef NO_GPIO_REQUEST_ARRAY
	ret = gpio_request(leds[0].gpio, leds[0].label);
#else
	ret = gpio_request_array(leds, ARRAY_SIZE(leds));
#endif

	/* エラーなら関数を終了する */
	if (ret) {
		pr_err("Unable to request GPIOs for LEDs: %d\n", ret);
		return ret;
	}

	/* ボタンGPIOのリクエスト */
#ifdef NO_GPIO_REQUEST_ARRAY
	/* GPIO 17をリクエスト */
	ret = gpio_request(buttons[0].gpio, buttons[0].label);
	if (ret) {
		pr_err("Unable to request GPIOs for BUTTONs: %d\n", ret);
		goto fail1;
	}

	/* GPIO 18をリクエスト */
	ret = gpio_request(buttons[1].gpio, buttons[1].label);
	if (ret) {
		pr_err("Unable to request GPIOs for BUTTONs: $d\n", ret);
		got fail2;
	}
#else
	/* gpio_request_array()を使って, すべてのボタンGPIOをリクエスト */
	ret = gpio_request_array(buttons, ARRAY_SIZE(buttons));
	if (ret) {
		pr_err("Unable to request GPIOs for BUTTONs: %d\n", ret);
		goto fail1;
	}
#endif

	/* ボタン1の状態を取得(0=押されていない, 1=押されている) */
	pr_info("Current button1 value: %d\n", gpio_get_value(buttons[0].gpio));

	/* GPIO 17のIRQ番号を取得する */
	ret = gpio_to_irq(buttons[0].gpio);
	/* IRQ番号取得に失敗 */
	if (ret < 0) {
		pr_err("Unable to request IRQ: %d\n", ret);
#ifdef NO_GPIO_REQUEST_ARRAY
		goto fail3;
#else
		goto fail2;
#endif
	}

	/* 成功した場合は, 戻り値がIRQ番号となるため, button_irqs[0]に保存する */
	button_irqs[0] = ret;
	pr_info("Successfully requested BUTTON1 IRQ # %d\n", button_irqs[0]);

	/**
	 * ボタン1のIRQハンドラを登録
	 * - button_irqs[0]: ボタン1のIRQ
	 * - button_isr: 割り込みハンドラ
	 * - IRQF_TRIGGER_RISING | IRQF_TRIGGER_FAILING: 立ち上がり・立ち下がりエッジの両方で割り込み発生
	 * - "gpiomod#button1": 識別用の名前
	 * - NULL: データなし
	 */
	ret = request_irq(button_irqs[0], button_isr,
						IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						"gpiomod#button1", NULL);
	
	/* IRQ登録に失敗 */
	if (ret) {
		pr_err("Unable to request IRQ: %d\n", ret);
#ifdef NO_GPIO_REQUEST_ARRAY
		goto fail3;
#else
		goto fail2;
#endif
	}

	/* GPIO 18のIRQ番号を取得 */
	ret = gpio_to_irq(buttons[1].gpio);
	/* IRQ番号の取得に失敗 */
	if (ret < 0) {
		pr_err("Unable to request IRQ: %d\n", ret);
#ifdef NO_GPIO_REQUEST_ARRAY
		goto fail3;
#else
		goto fail2;
#endif
	}

	/* 成功した場合は, 戻り値がIRQ番号となるため, button_irqs[1]に保存する */
	button_irqs[1] = ret;
	pr_info("Successfully requested BUTTON2 IRQ # %d\n", button_irqs[1]);

	/**
	 * ボタン2のIRQハンドラを登録
	 * - button_irqs[1]: ボタン2のIRQ
	 * - button_isr: 割り込みハンドラ
	 * - IRQF_TRIGGER_RISING | IRQF_TRIGGER_FAILING: 立ち上がり・立ち下がりエッジの両方で割り込み発生
	 * - "gpiomod#button2": 識別用の名前
	 * - NULL: データなし
	 */
	ret = request_irq(button_irqs[1], button_isr,
						IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						"gpiomod#button2", NULL);

	if (ret) {
		pr_err("Unable to request IRQ: %d\n", ret);
#ifdef NO_GPIO_REQUEST_ARRAY
		goto fail4;
#else
		goto fail3;
#endif
	}

	return 0;

	/* エラー時にリソースを解放するための処理 */
#ifdef NO_GPIO_REQUEST_ARRAY
fail4:
	free_irq(button_irqs[0], NULL);

fail3:
	gpio_free(buttons[1].gpio);

fail2:
	gpio_free(buttons[0].gpio);

fail1:
	gpio_free(leds[0].gpio);
#else
fail3:
	free_irq(button_irqs[0], NULL);

fail2:
	gpio_free_array(buttons, ARRAY_SIZE(buttons));

fail1:
	gpio_free_array(leds, ARRAY_SIZE(leds));
#endif

	return ret;
}

/**
 * @brief カーネルモジュールクリーンアップ関数
 */
static void __exit intrpt_exit(void) {
	pr_info("%s\n", __func__);

	/* 登録した割り込みを解除する */
	free_irq(button_irqs[0], NULL);
	free_irq(button_irqs[1], NULL);

	/* LEDを確実にOFFにする */
#ifdef NO_GPIO_REQUEST_ARRAY
	gpio_set_value(leds[i].gpio, 0);
#else
	int i;
	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		gpio_set_value(leds[i].gpio, 0);
	}
#endif

	/* 使用したGPIOを解放する */
#ifdef NO_GPIO_REQUEST_ARRAY
	gpio_free(leds[0].gpio);
	gpio_free(buttons[0].gpio);
	gpio_free(buttons[1].gpio);
#else
	gpio_free_array(leds, ARRAY_SIZE(leds));
	gpio_free_array(buttons, ARRAY_SIZE(buttons));
#endif
}

module_init(intrpt_init);
module_exit(intrpt_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Handle some GPIO interrupts");
