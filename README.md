## Linuxカーネル作ってみる

参考資料
- https://qiita.com/nouernet/items/77bdea90e5423d153c8d
- https://sysprog21.github.io/lkmpg/

---

## カーネルモジュールのロードとアンロード

例としてhello_world

使用したいカーネルモジュールのディレクトリに移動
```
$ cd hello_world
```

makeを実行し,ビルドする
```
$ make all
```

hello.koモジュールがコンパイルされる

カーネルにロードする
```
$ sudo insmod hello.ko
```

カーネルモジュールの一覧を確認やログを確認することでロードされていることを確認する
```
# ロードされているカーネルモジュールから,helloを含んでいるものを検索する
$ lsmod | grep hello

# 直近の末尾のログを確認する
$ journalctl -f

# 1時間以内のログから検索する
$ journalctl --since "1 hour ago" | grep kernel
```

カーネルからアンロードする
```
$ sudo rmmod hello
```
