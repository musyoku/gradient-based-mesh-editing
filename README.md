# :construction: Work in Progress :construction:

## Gradient-based 3D Mesh Editing

[Neural 3D Mesh Renderer](https://arxiv.org/abs/1711.07566)で提案されたGradient-based mesh editingのC++/CUDA実装です。

CPU・GPU両方で動きます。

詳細な使い方は`python`と`js`ディレクトリのREADMEをお読みください。

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