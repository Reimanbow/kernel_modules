## Linuxカーネル作ってみる

参考資料
- https://qiita.com/nouernet/items/77bdea90e5423d153c8d
- https://sysprog21.github.io/lkmpg/

---

## ディレクトリ

- basic: カーネルログにメッセージを出力する程度のモジュール
- cdev: キャラクタデバイス
- procfs: /procに情報を出力する

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
```

カーネルからアンロードする
```
$ sudo rmmod hello_1
```
