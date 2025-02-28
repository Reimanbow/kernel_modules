## Linuxカーネル作ってみる

参考資料
- https://sysprog21.github.io/lkmpg/

---

## ディレクトリ

- basic: カーネルログにメッセージを出力する程度のモジュール
- cdev: キャラクタデバイス
- procfs: /procの制御などを行うモジュール
- syscall: システムコール関連
- sysfs: /sysの制御などを行うモジュール
- thread: マルチスレッド, 排他制御

## カーネルモジュールのロードとアンロード

例としてhello-1

使用したいカーネルモジュールのディレクトリに移動
```
$ cd basic/hello-1
```

makeを実行し,ビルドする
```
$ make all
```

hello-1.koモジュールがコンパイルされる

カーネルにロードする
```
$ sudo insmod hello-1.ko
```

カーネルモジュールの一覧を確認やログを確認することでロードされていることを確認する
```
# ロードされているカーネルモジュールから,helloを含んでいるものを検索する
$ lsmod | grep hello_1

# 直近の末尾のログを確認する
$ journalctl -f

# 1時間以内のログから検索する
$ journalctl --since "1 hour ago" | grep kernel

# 直近のカーネルのログを確認する
$ sudo dmesg -w
```

カーネルからアンロードする
```
$ sudo rmmod hello_1
```
