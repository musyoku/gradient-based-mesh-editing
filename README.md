# :construction: Work in Progress :construction:

## Gradient-based 3D Mesh Editing

[Neural 3D Mesh Renderer](https://arxiv.org/abs/1711.07566)で提案されたGradient-based mesh editingのC++/CUDA実装です。

CPU・GPU両方で動きます。

## インストール

**pybind11**

CPU版はC++で実装しているのでビルドする必要があります。

まず以下のコマンドでpybind11をインストールします。

```
pip3 install pybind11 --user
```

**ビルド**

以下のコマンドで共有ライブラリを作ります。

```
cd gradient_based_editing
make
```

**ビューワ**

可視化を行うにはビューワをビルドする必要があります。

マルチスレッドでOpenGLを動かしているのでmacOSでは動きません。

```
cd viewer
make
```

![preview](https://qiita-image-store.s3.amazonaws.com/0/109322/f7521073-33c1-c204-c1ae-6c40f0e70537.gif)