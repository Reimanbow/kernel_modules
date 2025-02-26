/**
 * @file syscall-steal.c
 * 
 * システムコールを置き換える
 */
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/version.h>

/* カレントプロセスで, カレントユーザが誰であるかを知るために必要 */
#include <linux/sched.h>
#include <linux/uaccess.h>

/**
 * sys_call_tableへのアクセス方法は, カーネル内部の変更に伴って変化する
 * 
 * - v5.4以前: 手動シンボル検索
 * - v5.5からv5.6: kallsyms_lookup_name()を使用
 * - v5.7以降: Kprobesまたは特定のカーネルモジュールパラメータ
 */

/* ksys_close()システムコールのカーネル内呼び出しはLinux v5.11+で削除された */
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 7, 0))

/**
 * Linux v5.4以前
 * 手動でsys_call_tableのシンボルを検索する方法を使用する
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0)
#define HAVE_KSYS_CLOSE 1
/* sys_call_tableにアクセスできる */
#include <linux/syscalls.h>

/**
 * Linux v5.5~v5.6
 * kallsyms_lookup_name()を使用してsys_call_tableのアドレスを取得可能
 * v5.7以降ではkallsyms_lookup_name()が非公開になり使えない
 */
#else
#include <linux/kallsyms.h>
#endif /* Version < v5.4 */

/**
 * Linux v5.7以降
 */
#else

/**
 * CONFIG_KPROBESが有効の場合は, Kprobesを使用する
 */
#if defined(CONFIG_KPROBES)
#define HAVE_KPROBES 1

#if defined(CONFIG_X86_64)
/**
 * syscallテーブルを使用してシステムコールをインターセプトしようとしたが上手く行かなかった場合,
 * Kprobesを使用してシステムコールをインターセプトすることができる.
 * USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALLを1に設定して,
 * システムコールの前にプリハンドラを登録する
 */

/**
 * USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALLについて
 * 0に設定すると, 通常のKprobesハンドラを使う
 * 1に設定すると, システムコールの前にフックできる
 */
#define USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALL 1
#endif
#include <linux/kprobes.h>

/**
 * KPROBESが無効な場合, sys_call_tableのアドレスを手動で指定
 * symというモジュールパラメータを使用し, /proc/kallsymsや/boot/System.mapからアドレスを取得
 */
#else
#define HAVE_PARAM 1
#include <linux/kallsyms.h>
/**
 * sys_call_tableのアドレスで, "/boot/System.map"または"/proc/kallsyms"を
 * 検索して得られる. カーネルバージョンがv5.7+の場合, CONFIG_KPROBESがなければ,
 * このパラメータを入力するが, モジュールがすべてのメモリを検索する
 */

static unsigned long sym = 0;
module_param(sym, ulong, 0644);
#endif /* CONFIG_KPROBES */

#endif /* Version < v5.7 */

/* スパイしたいUIDはコマンドラインから入力する */
static uid_t uid = -1;
/* カーネルモジュールのパラメータとしてinsmodコマンドで指定可能 */
module_param(uid, int, 0644);

/* Kprobesを使用する場合. Kprobesを使ってopenat()の呼び出しをフックする */
#if USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALL

/**
 * syscall_symは, スパイするシステムコールのシンボル名である
 * デフォルトは"__x64_sys_openat"で, moduleパラメータで変更できる
 * syscallのシンボル名は/proc/kallsymsで調べることができる
 */
static char *syscall_sym = "__x64_sys_openat";
module_param(syscall_sym, charp, 0644);

/**
 * @brief openat()の呼び出しをフックする関数
 */
static int sys_call_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs) {
	/**
	 * 現在のプロセスの実行ユーザUIDを取得し,
	 * 監視対象のユーザがopenat()を呼び出した場合, pr_infoでカーネルログに記録
	 */
	if (__kuid_val(current_uid()) != uid) {
		return 0;
	}

	pr_info("%s called by %d\n", syscall_sym, uid);
	return 0;
}

/**
 * @struct kprobe
 * symbol_name = __x64_sys_openat -> openat()にフックを設定
 * pre_handler = sys_call_kprobe_pre_handler -> システムコール実行前に実行する
 */
static struct kprobe syscall_kprobe = {
	.symbol_name = "__x64_sys_openat",
	.pre_handler = sys_call_kprobe_pre_handler,
};

/* Kprobesが無効なとき */
#else

/* sys_call_tableへのポインタ(ハイジャック用) */
static unsigned long **sys_call_table_stolen;

/**
 * 元のシステムコールへのポインタ. 元の関数(sys_openat)を呼び出すのではなく
 * これを保持する理由は, 他の誰かが我々の前にシステムコールを置き換えたかもしれないからである
 * なぜなら, 他のモジュールが先にsys_openatを置き換えた場合, 我々が挿入したときに
 * そのモジュールの関数を呼び出すことになり, 我々より先に削除される可能性があるからである
 * 
 * もう一つの理由は, sys_openatを取得できないことである
 * これは静的変数なのでエクスポートされない. sys_openatを直接呼ぶことができない
 */
#ifdef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
static asmlinkage long (*orginal_call)(const struct pt_regs *);
#else
static asmlinkage long (*orignal_call)(int, const char __user *, int, umode_t);
#endif

/**
 * @brief sys_openatを置き換える関数
 * 引数の数と型を含む正確なプロトタイプを見つけるには
 * まず元の関数を見つける(fs/open.cにある)
 * 
 * これは現在のバージョンのカーネルに縛られることを意味する
 * 実際には, システムコールはほとんど変更されない
 * 
 * カーネルバージョンにより, 異なる関数プロトタイプを使用
 * - CONFIG_ARCH_HAS_SYSCALL_WRAPPERが定義されている場合, pt_regs構造体を使う(CPUのレジスタ情報)
 * - それ以外は通常のopenat()の引数(ファイルディスクリプタやファイル名など)を受け取る
 */
#ifdef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
static asmlinkage long our_sys_openat(const struct pt_regs *regs)
#else
static asmlinkage long our_sys_openat(int dfd, const char __user *filename, int flags, umode_t mode)
#endif
{
	int i = 0;
	char ch;

	/**
	 * 現在のプロセスの実行ユーザUIDを取得し
	 * 監視対象のUIDと一致しなければ, 通常のopenat()を実行する
	 */
	if (__kuid_val(current_uid()) != uid) {
		goto orig_call;
	}

	pr_info("Opened file by %d: ", uid);
	/* get_user()を使ってユーザ空間からfilenameを取得 */
	do {
#ifdef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
		get_user(ch, (char __user *)regs->si + i);
#else
		get_user(ch, (char __user *)filename + i);
#endif
		i++;
		pr_info("%c", ch);
	} while (ch != 0);
	pr_info("\n");

/* デフォルトのsys_openatを呼び出す. さもなければファイルを開く能力を失う */
orig_call:
#ifdef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
	return original_call(regs);
#else
	return original_call(dfd, filename, flags, mode);
#endif
}

/**
 * @brief sys_call_tableを取得する関数
 * カーネルバージョンによって取得方法が異なる
 */
static unsigned long **acquire_sys_call_table(void) 
{
/**
 * Linux v5.4以前
 * sys_call_tableのアドレスをメモリ内検索
 * - PAGE_OFFSETからULLONG_MAXまでメモリを操作
 * - sct[__NR_close] == (unsigned long *)ksys_closeの条件を満たすアドレスを探す
 * - ksys_closeはclose()のカーネル内部関数->sys_call_tableのclose()のエントリが
 *                                      ksys_closeならsys_call_tableの可能性が高い
 */
#ifdef HAVE_KSYS_CLOSE
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *)ksys_close) {
			return sct;
		}

		offset += sizeof(void *);
	}

	return NULL;
#endif

/**
 * Linux v5.7以降(推奨ではない?)
 * カーネル5.7以降では, sys_call_tableのアドレスを直接取得できなくなった
 * symという変数に手動でsys_call_tableのアドレスを渡す必要がある
 * - /boot/System.mapまたは/proc/kallysymsを調べてsys_call_tableのアドレスを取得
 * sprint_symbol(symbol, sym);を使い, symが正しいsys_call_tableかチェック
 */
#ifdef HAVE_PARAM
	const char sct_name[15] = "sys_call_table";
	char symbol[40] = {0};

	if (sym == 0) {
		pr_alert("For Linux v5.7+, Kprobes is the preferable way to get symbol.\n");
		pr_info("If Kprobes is absent, you have to specify the address of sys_call_table symbol\n");
		pr_info("by /boot/System.map or /proc/kallsyms, which contains all the symbol addresses, info sym parameter.\n");
		return NULL;
	}
	sprint_symbol(symbol, sym);
	if (!strncmp(sct_name, symbol, sizeof(sct_name) - 1)) {
		return (unsigned long **)sym;
	}

	return NULL;
#endif

/**
 * Linux 5.7以降(推奨)
 * kprobesを利用してkallsyms_lookup_name()を取得
 * kallsyms_lookup_name()を使うとカーネル内部の関数のアドレスを取得可能
 * - "sys_call_table"をkallsyms_lookup_name()に渡してsys_call_tableのアドレスを取得
 * カーネル5.7以降ではsys_call_tableが推奨されないため, Kprobesが推奨される
 */
#ifdef HAVE_KPROBES
	unsigned long (*kallsyms_lookup_name)(const char *name);
	struct kprobe kp = {
		.symbol_name = "kallsym_lookup_name",
	};

	if (register_kprobe(&kp) < 0) {
		return NULL;
	}
	kallsyms_lookup_name = (unsigned long (*)(const char *name))kp.addr;
	unregister_kprobe(&kp);
#endif
	return (unsigned long **)kallsyms_lookup_name("sys_call_table");
}

/* カーネルの活気込み保護の無効化/有効化 */
/* v5.3以降: CR0レジスタを書き換えるために, インラインアセンブリを使用*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
/**
 * @brief CR0レジスタの書き込み
 */
static inline void __write_cr0(unsigned long cr0)
{
	asm volatile("mov %0,%%cr0" : "+r"(cr0) : : "memory");
}
/* v5.2以前: write_cr0()というカーネルの既存関数を使う */
#else
#define __write_cr0 write_cr0
#endif

/**
 * @brief 書き込み保護の有効化
 */
static void enable_write_protection(void) {
	/* CR0レジスタを取得 */
	unsigned long cr0 = read_cr0();
	/* 16ビット目(WP)をセットして書き込み保護を有効化 */
	set_bit(16, &cr0);
	__write_cr0(cr0);
}

/**
 * @brief 書き込み保護の無効化
 */
static void disable_write_protection(void) {
	/* CR0レジスタを取得 */
	unsigned long cr0 = read_cr0();
	/* 16ビット目(WP)をクリアして書き込み保護を無効化 */
	clear_bit(16, &cr0);
	__write_cr0(cr0);
}
#endif /* USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALL */

/**
 * @brief システムコールフックの初期化(カーネルモジュールの初期化関数)
 */
static int __init syscall_steal_start(void) 
{
 /* Kprobesを使ってシステムコールをフックする */
#if USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALL
	int err;
	/* モジュールパラメータからシンボル名を使用 */
	syscall_kprobe.symbol_name = syscall_sym;
	/* register_kprobe()でsyscall_kprobeを登録 */
	err = register_kprobe(&syscall_kprobe);
	if (err) {
		pr_err("register_kprobe() on %s failed: %d\n", syscall_sym, err);
		pr_err("Please check the symbol name from 'syscall_sym' parameter.\n");
		return err;
	}
/* sys_call_tableを書き換える場合 */
#else
	/* sys_call_tableのアドレスを取得 */
	if (!(sys_call_table_stolen = acquire_sys_call_table())) {
		return -1;
	}

	/* 書き込み保護を無効化 */
	disable_write_protection();

	/* openatエントリの書き換え */
	/* original_callに元のopenat()のアドレスを保存 */
	original_call = (void *)sys_call_table_stolen[__NR_openat];

	/* sys_call_tableに置き換える */
	sys_call_table_stolen[__NR_openat] = (unsigned *)our_sys_openat;

	/* 書き込み保護を有効化 */
	enable_write_protection();
#endif

	pr_info("Spying on UID: %d\n", uid);
	return 0;
}

/**
 * @brief システムコールフックの解除(カーネルモジュールのクリーンアップ関数)
 */
static void __exit syscall_steal_end(void)
{
/* Kprobesを使う場合, 登録したKprobeを解除 */
#if USE_KPROBES_PRE_HANDLER_BEFORE_SYSCALL
	unregister_kprobe(&syscall_kprobe);
/* sys_call_tableを元に戻す場合 */
#else
	/* sys_call_tableが取得できていればなにもしない */
	if (!sys_call_table_stolen) {
		return 0;
	}

	/* 他のモジュールがopenat()を書き換えているか確認 */
	if (sys_call_table_stolen[__NR_openat] != (unsigned long *)our_sys_openat) {
		pr_alert("Somebody else also played with the ");
		pr_alert("open system call\n");
		pr_alert("The system may be left in ");
		pr_alert("an unstable state.\n");
	}

	/* 書き込み保護を無効化 */
	disable_write_protection();

	/* sys_call_tableを元に戻す */
	sys_call_table_stolen[__NR_openat] = (unsigned long *)original_call;

	/* 書き込み保護を有効化 */
	enable_write_protection();
#endif
	/**
	 * モジュール削除後, 2秒間スリープ
	 * openat()にアクセスしているプロセスがいる場合, 競合を防ぐため
	 */
	msleep(2000);
}

module_init(syscall_steal_start);
module_exit(syscall_steal_end);

MODULE_LICENSE("GPL");