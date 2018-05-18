## インストール

**Cython**

CPU版はC++で実装しているのでCythonでビルドする必要があります。

Cythonをインストールするには以下のコマンドを実行します。

```
pip3 install cython --user
```

**レンダラーのビルド**

以下のコマンドで共有ライブラリを作ります。

```
python3 setup.py build_ext -i
```