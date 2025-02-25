/**
 * @file ioctl.c
 * 
 * ユーザプログラムとカーネルの間で, 入出力制御を使用するキャラクタデバイスドライバ
 * ユーザプログラムがこのドライバを通じて特定のデバイスにデータを読み書きできる
 */
#include "ioctl-1.h"

//! デバイスのメジャー番号(動的に決定される)
unsigned int test_ioctl_major = 0;

//! 作成するデバイスの数(1つ)
unsigned int num_of_dev = 1;

//! キャラクタデバイスの構造体
static struct cdev test_ioctl_cdev;

//! 数値データのやり取りに使う変数
int ioctl_num = 0;

/**
 * @brief ioctlシステムコールの処理
 * ユーザが指定したcmdに応じた操作を実行する
 * ユーザ空間<->カーネル空間
 * 
 * @param file: オープンされているデバイスファイル
 * @param cmd: ioctlコマンド番号
 * @param arg: ユーザ空間から渡されたデータ(ポインタまたは数値)
 * 
 * @return 0なら成功
 */
static long test_ioctl_ioctl(struct file *file, unsigned int cmd,
							 unsigned long arg)
{
	struct test_ioctl_data *ioctl_data = file->private_data;
	int retval = 0;
	unsigned char val;
	struct ioctl_arg data;
	memset(&data, 0, sizeof(data));

	/* ユーザが指定したcmdに応じた操作を実行する */
	switch (cmd) {
	/* ユーザからvalを受け取り, デバイスに書き込む */
	case IOCTL_VALSET:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
			retval = -EFAULT;
			goto done;
		}

		pr_alert("IOCTL set val:%x .\n", data.val);
		/* ioctl_data->valの書き込みを保護 */
		write_lock(&ioctl_data->lock);
		ioctl_data->val = data.val;
		/* ロックを解除 */
		write_unlock(&ioctl_data->lock);
		break;

	/* デバイスに保存されているvalをユーザに返す */
	case IOCTL_VALGET:
		/* 読み取りを保護 */
		read_lock(&ioctl_data->lock);
		val = ioctl_data->val;
		/* ロックを解除 */
		read_unlock(&ioctl_data->lock);
		data.val = val;

		/* ユーザ空間へvalをコピー */
		if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
			retval = -EFAULT;
			goto done;
		}

		break;

	/* ioctl_numをユーザに返す */
	case IOCTL_VALGET_NUM:
		/* ioctl_numをユーザ空間にコピー */
		retval = __put_user(ioctl_num, (int __user *)arg);
		break;

	/* ioctl_numにユーザが指定した値を渡す */
	case IOCTL_VALSET_NUM:
		/* ioctl_numにargをセット */
		ioctl_num = arg;
		break;

	default:
		retval = -ENOTTY;
	}

done:
	return retval;
}

/**
 * @brief ユーザがreadしたとき, デバイスに保存されているvalを読み取る
 * カーネル空間->ユーザ空間
 * 
 * @param file: オープンされているデバイスファイル
 * @param buf: ユーザ空間のバッファ(データをコピーする先)
 * @param count: 読み込むデータのバイト数
 * @param f_pos: ファイルのオフセット
 * 
 * @return 読み込んだバイト数
 */
static ssize_t test_ioctl_read(struct file *file, char __user *buf,
							   size_t count, loff_t *f_pos)
{
	struct test_ioctl_data *ioctl_data = file->private_data;
	unsigned char val;
	int retval;
	int i = 0;

	/* ロックをかけ, デバイスのvalを取得する */
	read_lock(&ioctl_data->lock);
	val = ioctl_data->val;
	read_unlock(&ioctl_data->lock);

	/* ユーザプロセスにデータを渡す */
	for (; i < count; i++) {
		if (copy_to_user(&buf[i], &val, 1)) {
			retval = -EFAULT;
			goto out;
		}
	}

	retval = count;
out:
	return retval;
}

/**
 * @brief デバイスをcloseしたときに呼ばれる
 */
static int test_ioctl_close(struct inode *inode, struct file *file) {
	pr_alert("%s call.\n", __func__);

	/* 確保したメモリを解放する */
	if (file->private_data) {
		kfree(file->private_data);
		file->private_data = NULL;
	}

	return 0;
}

/**
 * @brief ユーザがデバイスをopenしたときに呼ばれる
 */
static int test_ioctl_open(struct inode *inode, struct file *file) {
	struct test_ioctl_data *ioctl_data;

	pr_alert("%s call.\n", __func__);

	/* メモリを確保し, デバイスのデータを初期化する */
	ioctl_data = kmalloc(sizeof(struct test_ioctl_data), GFP_KERNEL);

	if (ioctl_data == NULL) {
		return -ENOMEM;
	}

	/* lockを初期化する */
	rwlock_init(&ioctl_data->lock);
	ioctl_data->val = 0xFF;

	/* file->private_data内に保持する */
	file->private_data = ioctl_data;

	return 0;
}

/**
 * @struct file_operations
 */
static struct file_operations fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	.owner = THIS_MODULE,
#endif
	.open = test_ioctl_open,
	.release = test_ioctl_close,
	.read = test_ioctl_read,
	.unlocked_ioctl = test_ioctl_ioctl,
};

/**
 * @brief モジュールの初期化
 */
static int __init ioctl_init(void) {
	dev_t dev;
	int alloc_ret = -1;
	int cdev_ret = -1;

	/* 動的にメジャー番号を取得し, デバイスを登録 */
	alloc_ret = alloc_chrdev_region(&dev, 0, num_of_dev, DRIVER_NAME);

	if (alloc_ret) {
		goto error;
	}

	/* cdev構造体を初期化し, fopsを設定 */
	test_ioctl_major = MAJOR(dev);
	cdev_init(&test_ioctl_cdev, &fops);

	/* カーネルにデバイスを追加 */
	cdev_ret = cdev_add(&test_ioctl_cdev, dev, num_of_dev);

	if (cdev_ret) {
		goto error;
	}

	pr_alert("%s driver(major: %d) installed.\n", DRIVER_NAME, test_ioctl_major);
	return 0;
error:
	if (cdev_ret == 0) {
		cdev_del(&test_ioctl_cdev);
	}
	if (alloc_ret == 0) {
		unregister_chrdev_region(dev, num_of_dev);
	}
	return -1;
}

/**
 * @brief モジュールの削除
 */
static void __exit ioctl_exit(void) {
	dev_t dev = MKDEV(test_ioctl_major, 0);

	/* デバイスを削除 */
	cdev_del(&test_ioctl_cdev);

	/* メジャー番号を解放 */
	unregister_chrdev_region(dev, num_of_dev);
	pr_alert("%s driver removed.\n", DRIVER_NAME);
}

module_init(ioctl_init);
module_exit(ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("This is test_ioctl module");