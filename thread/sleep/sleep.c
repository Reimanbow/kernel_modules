/**
 * @file sleep.c
 * 
 * /procにファイルを作成し, 複数のプロセスが同時にそれを開こうとしたとき, 1つを除いてスリープ状態にする
 */
#include <linux/atomic.h>	/* アトミック操作を行うための関数を提供する */
#include <linux/fs.h>		/* ファイルシステム関連の構造体や関数を定義する */
#include <linux/kernel.h>	/* カーネル内で一般的に使用される関数やマクロ */
#include <linux/module.h>	/* カーネルモジュールを作成する際に必要なヘッダ */
#include <linux/printk.h>	/* カーネルのログメッセージを出力するprintk()を提供する */
#include <linux/proc_fs.h>	/* /procファイルシステムを操作するための関数や構造体を提供する */
#include <linux/types.h>	/* 型定義(size_tやloff_tなど)を提供する */
#include <linux/uaccess.h>	/* ユーザ空間とのデータのやり取りに使用される */
#include <linux/version.h>	/* カーネルのバージョンに応じたコンパイルを行う */
#include <linux/wait.h>		/* スリープやウェイクアップを制御するwait_event()などの関数を提供する */

#include <asm/current.h>	/* 現在のプロセス(currentマクロ)を取得するための定義が含まれる */
#include <asm/errno.h>		/* エラーコードの定義を提供する */

/**
 * v5.6以降では, proc_ops構造体を使用する
 * それ以前では, file_operationsを使う
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

/**
 * /proc/sleepに書き込まれたデータを一時的に保存するためのバッファ
 * module_input()で書き込まれ, module_output()で読み取られる
 */
#define MESSAGE_LENGTH 80
static char message[MESSAGE_LENGTH];

/* /proc/sleepファイルを作製するためのエントリ */
static struct proc_dir_entry *our_proc_file;
#define PROC_ENTRY_FILENAME "sleep"

/**
 * ファイル操作構造体を使うので, 特別なproc出力規定を使うことはできない
 * 標準の読み取り関数を使わなければならない
 * 
 * /proc/sleepが読み込まれると, 最後に/proc/sleepに書き込まれたメッセージを返す
 */
static ssize_t module_output(struct file *file, char __user *buf,
							 size_t len, loff_t *offset)
{
	/**
	 * /procファイルの読み込みは一般のファイルとは異なり, EOFが必要である
	 * finishedを用いることで, 一度読み込んだらEOFを返し, 次の読み込みで再びデータを提供できるようにする
	 */
	static int finished = 0;
	int i;
	char output_msg[MESSAGE_LENGTH + 30];

	/* ファイルの終了を示す0を返す */
	if (finished) {
		finished = 0;
		return 0;
	}

	/* messageの内容をoutput_msgにコピーする */
	sprintf(output_msg, "Last input:%s\n", message);
	for (i = 0; i < len && output_msg[i]; i++) {
		/* output_msgの内容をユーザ空間にコピーする */
		put_user(output_msg[i],buf + i);
	}

	finished = 1;
	return i;
}

/**
 * ユーザが/procファイルに書き込む際に, ユーザからの入力を受け取る
 * 
 * /proc/sleepに書き込まれた内容をmessageに保存する
 */
static ssize_t module_input(struct file *file, const char __user *buf,
							size_t length, loff_t *offset)
{
	int i;

	/* 入力をmessageに入れ, 後でmodule_outputがそれを使えるようにする */
	for (i = 0; i < MESSAGE_LENGTH - 1 && i < length; i++) {
		/* 1バイトずつ安全にコピーしている */
		get_user(message[i], buf + i);
	}
	/* 終端を追加する */
	message[i] = '\0';

	return i;
}

/* ファイルが現在誰かによって開かれている場合は1となる */
static atomic_t already_open = ATOMIC_INIT(0);

/**
 * ファイルを必要とするプロセスのキュー
 * waitq
 * - /proc/sleepを開くことを待機しているプロセスのキュー(待ち行列)
 * - wait_event_interruptible()によってスリープするプロセスを管理する
 */
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/* /proc/sleepがオープンされたときに呼び出される */
static int module_open(struct inode *inode, struct file *file) {
	/* 既に開かれていないかチェック */
	if (!atomic_cmpxchg(&already_open, 0, 1)) {
		/* ブロッキングせずに成功すれば, アクセスを許可する */
		try_module_get(THIS_MODULE);
		return 0;
	}

	/**
	 * O_NONBLOCKフラグのチェック
	 * O_NONBLOCKが指定されていた場合, ブロックせずにエラー -EAGAINを返す
	 * - これは他のプロセスが開いているから, すぐに再試行してという意味
	 */
	if (file->f_flags & O_NONBLOCK) {
		return -EAGAIN;
	}

	/* try_module_get(THIS_MODULE); でモジュールの使用カウントを増やして, 削除されないようにする */
	try_module_get(THIS_MODULE);

	/* already_openを再確認し, まだ1ならループを開始 */
	while (atomic_cmpxchg(&already_open, 0, 1)) {
		int i, is_sig = 0;

		/**
		 * already_openが0になるまで(他のプロセスが/proc/sleepを閉じるまで)スリープする
		 * wait_event_interruptible()はシグナルを受け取ると目覚める
		 */
		wait_event_interruptible(waitq, !atomic_read(&already_open));

		/* シグナルを受け取った場合の処理 */
		/* プロセスがブロックしていないシグナルを受け取ったかを確認する */
		for (i = 0; i < _NSIG_WORDS && !is_sig; i++) {
			is_sig = current->pending.signal.sig[i] & ~current->blocked.sig[i];
		}

		/**
		 * シグナルを受け取っていたらmodule_put(THIS_MODULE);
		 * でカーネルモジュールの参照カウントを減らし, エラー -EINTRを返す
		 */
		if (is_sig) {
			module_put(THIS_MODULE);
			return -EINTR;
		}
	}
	/* whileループを抜けると, already_openを1にすることに成功敷いているので, 開くことができる */

	return 0;
}

/**
 * ファイルがクローズしたときの処理
 */
static int module_close(struct inode *inode, struct file *file) {
	/**
	 * already_openを0に設定することで, waitqのプロセスの1つが
	 * already_openを1に戻し, ファイルを開くことができるようになる
	 */
	atomic_set(&already_open, 0);

	/**
	 * waitqの全プロセスを起こし, ファイルを待っているプロセスに渡せるようにする
	 * wait_event_interruptible()でスリープしていたプロセスが動き出す
	 */
	wake_up(&waitq);

	module_put(THIS_MODULE);

	return 0;
}

/**
 * v5.6以降ならproc_opsを使用する
 */
#ifdef HAVE_PROC_OPS
static const struct proc_ops file_ops_4_our_proc_file = {
	.proc_read = module_output,
	.proc_write = module_input,
	.proc_open = module_open,
	.proc_release = module_close,
	.proc_lseek = noop_llseek,
};
#else
static const struct file_operations file_ops_4_our_proc_file = {
	.read = module_output,
	.write = module_input,
	.open = module_open,
	.release = module_close,
	.lseek = noop_llseek,
}
#endif

/**
 * @brief カーネルモジュールの初期化関数
 * 
 * /procにファイルを登録する
 */
static int __init sleep_init(void) {
	/** 
	 * /proc/sleepを作成する
	 * - ファイル名: "sleep"
	 * - 権限: 0644
	 * - 親ディレクトリ: NULL(/proc/配下に作成される)
	 * - ファイル操作構造体: file_ops_4_our_proc_file
	*/
	our_proc_file = 
		proc_create(PROC_ENTRY_FILENAME, 0644, NULL, &file_ops_4_our_proc_file);
	
	/* proc_create()が失敗したら-ENOMEMを返す */
	if (our_proc_file == NULL) {
		pr_debug("Error: Could not initialize /proc/%s\n", PROC_ENTRY_FILENAME);
		return -ENOMEM;
	}
	/* ファイルのサイズを80バイトに設定 */
	proc_set_size(our_proc_file, 80);
	/* ファイルの所有者をroot:rootに変更 */
	proc_set_user(our_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID);

	pr_info("/proc/%s created\n", PROC_ENTRY_FILENAME);

	return 0;
}

/**
 * @brief カーネルモジュールのクリーンアップ関数
 * 
 * /procからファイルの登録を削除する
 */
static void __exit sleep_exit(void) {
	/* /proc/sleepを削除する */
	remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
	pr_debug("/proc/%s removed\n", PROC_ENTRY_FILENAME);
}

module_init(sleep_init);
module_exit(sleep_exit);

MODULE_LICENSE("GPL");