/**
 * @file gpio_sample.c
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/io.h>

/* procファイルで受け取る最大データサイズ */
#define MAX_USER_SIZE 1024
/* GPIOレジスタの物理アドレス */
#define BCM2711_GPIO_ADDRESS 0xfe200000
/* procファイルの名前 */
#define PROC_FILENAME "gpio_sample"

/* ユーザ空間から受け取るデータを一時的に格納 */
static char data_buffer[MAX_USER_SIZE + 1] = {0};

/* GPIOレジスタの仮想アドレスを格納 */
static unsigned int *gpio_registers = NULL;

/**
 * @brief GPIOピンをONにする
 * 
 * GPIOの設定
 * - GPIOはFunction Selectレジスタ(FSEL)で入力(0), 出力(1), 特殊機能(2~7)を選択
 * - 0x1C(GPSET0)を使ってピンをHIGH(1)にする
 * 
 * BCM2711のGPIOは, レジスタ経由で制御される
 * - GPFSELn: オフセットは0x00~0x14. GPIOのモード設定(入力/出力)
 * - GPSET0: オフセットは0x1C. GPIOをHIGHにする
 * - GPCLR0: オフセットは0x28. GPIOをOFFにする
 * 
 * Raspberry PiのGPIOは1ピンあたり3ビットの設定領域を持つ
 * - 入力モード: 000
 * - 出力モード: 001
 * - その他の特殊機能: 010~111
 */
static void gpio_pin_on(unsigned int pin) {
	/* ピン番号からFSELレジスタのインデックスを取得 */
	unsigned int fsel_index = pin / 10;
	/* ピン番号からレジスタ内のビット位置を計算 */
	unsigned int fsel_bitpos = pin % 10;
	/* FSELレジスタのポインタ */
	unsigned int* gpio_fsel = gpio_registers + fsel_index;
	/* GPIO_BASE + 0x1CはGPSET0(GPIO0~31のHIGH制御) */
	unsigned int* gpio_on_register = (unsigned int*)((char*)gpio_registers + 0x1c);

	/* FSELの3ビットをクリア */
	*gpio_fsel &= ~(7 << (fsel_bitpos * 3));
	/* GPIOを出力モードに設定 */
	*gpio_fsel |= (1 << (fsel_bitpos * 3));
	/* GPIOをHIGHにする */
	*gpio_on_register |= (1 << pin);

	return;
}

/**
 * @brief GPIOピンをOFFにする関数
 * 
 * 0x28(GPCLR0)を使ってピンをLOW(0)にする
 */
static void gpio_pin_off(unsigned int pin) {
	/* GPIO_BASE + 0x1CはGPCLR0(GPIO0~31のLOW制御) */
	unsigned int *gpio_off_register = (unsigned int*)((char*)gpio_registers + 0x28);
	/* GPIOをLOWにする */
	*gpio_off_register |= (1 << pin);
	return;
}

/**
 * @brief /proc/gpio_sampleへの書き込み
 * 
 * /proc/gpio_sampleに[pin番号,1]または[pin番号,0]を書き込むと, GPIOのON/OFFを制御
 */
static ssize_t driver_proc_write(struct file *file, const char __user *buffer,
								 size_t count, loff_t *offset)
{
	/* GPIOピン番号 */
	unsigned int pin = UINT_MAX;
	/* ON(1)またはOFF(0) */
	unsigned int value = UINT_MAX;

	/* ゼロクリアをして前回のデータを残さない */
	memset(data_buffer, 0x0, sizeof(data_buffer));

	if (count > MAX_USER_SIZE) {
		count = MAX_USER_SIZE;
	}

	/* ユーザ空間からのデータをカーネル空間にコピー */
	if (copy_from_user(data_buffer, buffer, count)) {
		return 0;
	}

	/* (17,1)のようなデータを取得できなければ失敗 */
	if (sscanf(data_buffer, "%d,%d", &pin, &value) != 2) {
		pr_alert("INVALID DATA FORMAT\n");
		return 0;
	}

	/* ピン番号が0~27以外ならエラー */
	if (pin > 27 || pin < 0) {
		pr_alert("INVALID PIN NUMBER\n");
		return count;
	}

	/* GPIOのON/OFFは1か0なので, それ以外は無視 */
	if (value != 0 && value != 1) {
		pr_alert("INVALID ON/OFF VALUE\n");
		return count;
	}

	if (value == 1) {
		gpio_pin_on(pin);
	} else if (value == 0) {
		gpio_pin_off(pin);
	}

	return count;
}

static struct proc_dir_entry *proc_file;

static struct proc_ops proc_fops = {
	.proc_write = driver_proc_write,
};

/**
 * @brief モジュールの初期化
 */
static int __init driver_sample_init(void) {
	pr_info("init GPIO pin");

	/**
	 * 物理アドレス0xFE200000をカーネル仮想アドレスにマッピング
	 * PAGE_SIZE(4096バイト)を確保
	 * GPIOレジスタを直接操作できるようにする
	 */
	gpio_registers = (int*)ioremap(BCM2711_GPIO_ADDRESS, PAGE_SIZE);

	if (gpio_registers == NULL) {
		pr_alert("failed to map GPIO memory to driver\n");
		return -ENOMEM;
	}

	pr_info("successfully mapped GPIO memory to the driver\n");

	/* /proc/gpio_sampleを作成する */
	proc_file = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);

	if (proc_file == NULL) {
		pr_alert("proc allocation failed\n");
		return -1;
	}

	pr_info("proc allocation successful\n");

	return 0;
}

/**
 * @brief モジュールの終了
 */
static void __exit driver_sample_exit(void) {
	/* GPIOのメモリマッピングを解除 */
	iounmap(gpio_registers);
	/* /proc/gpio_sampleを解除 */
	proc_remove(proc_file);

	pr_info("exit GPIO pin");
}

module_init(driver_sample_init);
module_exit(driver_sample_exit);

MODULE_LICENSE("GPL");