/**
 * @file bh_thread.c
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/version.h>

/**
 * カーネルバージョンが6.10.0以上なら
 * gpio_request_array()の代わりに, gpio_request()を使用する
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
#define NO_GPIO_REQUEST_ARRAY
#endif

/**
 * ボタン用のIRQ番号を格納する配列
 * 初期値-1は未定義を意味する
 */
static int button_irqs[] = { -1 };

/* GPIO17に接続されたボタン(入力用) */
static struct gpio buttons[] = {
	{ 17, GPIOF_IN, "INTRERUPPT TRIGGER BUTTON" },
};

/**
 * @brief 割り込みハンドラ(tophalf)
 * 
 * ボタンが押されると呼ばれる割り込み関数
 */
static irqreturn_t button_top_half(int irq, void *ident) {
	/* IRQ_WAKE_THREADを返して, bottomhalfを実行する */
	return IRQ_WAKE_THREAD;
}

/**
 * @brief 割り込みハンドラ(bottomhalf)
 * 
 * tophalfから呼び出され, 500msの遅延を発生させる
 */
static irqreturn_t button_bottom_half(int irq, void *ident) {
	pr_info("Bottom half task starts\n");
	mdelay(500);
	pr_info("Bottom half task ends\n");
	return IRQ_HANDLED;
}

/**
 * @brief カーネルモジュール初期化関数
 */
static int __init bottomhalf_init(void) {
	int ret = 0;

	/* 現在の関数名をカーネルログに出力する */
	pr_info("%s\n", __func__);

	/* ボタンGPIOのリクエスト */
#ifdef NO_GPIO_REQUEST_ARRAY
	/* GPIO17をリクエスト */
	ret = gpio_request(buttons[0].gpio, buttons[0].label);
	if (ret) {
		pr_err("Unable to request GPIOs for BUTTONs: %d\n", ret);
		return ret;
	}
#else
	/* gpio_request_array()を使って, すべてのボタンGPIOをリクエスト */
	ret = gpio_request_array(buttons, ARRAY_SIZE(buttons));
	if (ret) {
		pr_err("Unable to request GPIOs for BUTTONs: %d\n", ret);
		return ret;
	}
#endif

	/* ボタン1の状態を取得(0=押されていない, 1=押されている) */
	pr_info("Current button1 value: %d\n", gpio_get_value(buttons[0].gpio));

	/* GPIO17のIRQ番号を取得する */
	ret = gpio_to_irq(buttons[0].gpio);
	if (ret < 0) {
		pr_err("Unable to request IRQ: %d\n", ret);
		goto fail;
	}

	/* 成功した場合は, 戻り値がIRQ番号となるため, button_irqs[0]に保存する */	
	button_irqs[0] = ret;
	pr_info("Successfully requested BUTTON1 IRQ # %d\n", button_irqs[0]);

	/**
	 * request_thread_irq()を使って, ボタン1の割り込みハンドラを登録
	 * - button_irqs[0]: 
	 * - button_top_half: トップハーフ
	 * - button_bottom_half: ボトムハーフ(遅延処理)
	 * - IRQF_TRIGGER_RISING | IRQF_TRIGGER_FAILING: 立ち上がり・立ち下がりエッジの両方で割り込み発生
	 * - "gpiomod#button1": 識別用の名前
	 */
	ret = request_threaded_irq(button_irqs[0], button_top_half,
								button_bottom_half,
								IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
								"gpiomod#button1", &buttons[0]);

	/* IRQ登録に失敗 */
	if (ret) {
		pr_err("Unable to request IRQ: %d\n", ret);
		goto fail;
	}

	return 0;

	/* エラー時にリソースを解放するための処理 */
#ifdef NO_GPIO_REQUEST_ARRAY	
fail:
	gpio_free(buttons[0].gpio);	
#else
fail:
	gpio_free_array(buttons, ARRAY_SIZE(buttons));
#endif

	return ret;
}

static void __exit bottomhalf_exit(void) {
	pr_info("%s\n", __func__);

	/* 登録した割り込みを解除する */
	free_irq(button_irqs[0], NULL);
	
	/* 使用したGPIOを解放する */
#ifdef NO_GPIO_REQUEST_ARRAY
	gpio_free(buttons[0].gpio);
#else
	gpio_free_array(buttons, ARRAY_SIZE(buttons));
#endif
}

module_init(bottomhalf_init);
module_exit(bottomhalf_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Interrupt with top and bottom half");