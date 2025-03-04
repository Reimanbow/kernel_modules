/**
 * @file mmap_sample.c
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME "mmap_test"
#define MEM_SIZE PAGE_SIZE /* 1ページ(4096バイト) */

/* カーネルメモリを保持するポインタ */
static char *kernel_buffer;

/**
 * @brief mmap()ハンドラ
 */
static int mmap_test_mmap(struct file *filp, struct vm_area_struct *vma) {
	unsigned long pfn;
	/* ユーザ空間が要求したサイズを取得 */
	size_t size = vma->vm_end - vma->vm_start;

	/* マッピング要求が大きすぎる */
	if (size > MEM_SIZE) {
		return -EINVAL;
	}

	/**
	 * virt_to_phys(kernel_buffer) >> PAGE_SHIFTで
	 * - kernel_bufferの物理ページ番号(pfn)を取得
	 * - PAGE_SHIFTは4096バイトのサイズを考慮
	 */
	pfn = slow_virt_to_phys(kernel_buffer) >> PAGE_SHIFT;

	/**
	 * ユーザプロセスの仮想メモリに物理ページをマッピング
	 * kmalloc()で確保したメモリは通常のmmap()では使えない
	 * そのため, 物理アドレスを直接ユーザ空間にマッピングする必要がある
	 */
	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	return 0;
}

/**
 * @brief open()ハンドラ
 */
static int mmap_test_open(struct inode *inode, struct file *filp) {
	return 0;
}

/**
 * @brief release()ハンドラ
 */
static int mmap_test_release(struct inode *inode, struct file *filp) {
	return 0;
}

/* file_operations構造体 */
static const struct file_operations mmap_test_fops = {
	.owner = THIS_MODULE,
	.mmap = mmap_test_mmap,
	.open = mmap_test_open,
	.release = mmap_test_release,
};

/* デバイス登録 */
static struct miscdevice mmap_test_device = {
	/* 動的にデバイス番号を割り当て */
	.minor = MISC_DYNAMIC_MINOR,
	/* /dev/mmap_testという名前のデバイス */
	.name = DEVICE_NAME,
	/* ファイル操作を指定 */
	.fops = &mmap_test_fops,
};

/**
 * @brief モジュール初期化
 */
static int __init mmap_test_init(void) {
	int ret;

	/* 1ページ分のカーネルメモリを確保 */
	kernel_buffer = kmalloc(MEM_SIZE, GFP_KERNEL);
	if (!kernel_buffer) {
		return -ENOMEM;
	}

	memset(kernel_buffer, 0, MEM_SIZE);

	/* デバイス登録 */
	ret = misc_register(&mmap_test_device);
	if (ret) {
		kfree(kernel_buffer);
		return ret;
	}

	pr_info("mmap_test module loaded\n");
	return 0;
}

/**
 * @brief モジュール終了
 */
static void __exit mmap_test_exit(void) {
	/* /dev/mmap_testを作成 */
	misc_deregister(&mmap_test_device);
	kfree(kernel_buffer);
	pr_info("mmap_test module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mmap test module");

module_init(mmap_test_init);
module_exit(mmap_test_exit);