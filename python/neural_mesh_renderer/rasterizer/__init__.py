import chainer
from .forward import forward_face_index_map_cpu
from .backward import backward_silhouette_cpu

class Rasterize(chainer.Function):
    def __init__(self, image_size, z_min, z_max):
        self.image_size = image_size
        self.z_min = z_min
        self.z_max = z_max

    # 各画素について最前面にある面のインデックスを探索する
    def forward_face_index_map_gpu(self):
        pass

    # 各画素について最前面にある面のインデックスを探索する
    def forward_face_index_map_cpu(self):
        pass

    def forward_gpu(self, inputs):
        pass

    def forward_cpu(self, inputs):
        pass