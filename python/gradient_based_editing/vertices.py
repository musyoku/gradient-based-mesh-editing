import math, chainer


def angle_to_radian(angle):
    return angle / 180.0 * math.pi


def rotate_x(vertices, angle):
    xp = chainer.cuda.get_array_module(vertices)
    rad = math.pi * (angle % 360) / 180.0
    rotation_mat = xp.asarray([
        [1, 0, 0],
        [0, math.cos(rad), -math.sin(rad)],
        [0, math.sin(rad), math.cos(rad)],
    ])
    vertices = xp.dot(vertices, rotation_mat.T)
    return vertices.astype(xp.float32)


def rotate_y(vertices, angle):
    xp = chainer.cuda.get_array_module(vertices)
    rad = math.pi * (angle % 360) / 180.0
    rotation_mat = xp.asarray([
        [math.cos(rad), 0, math.sin(rad)],
        [0, 1, 0],
        [-math.sin(rad), 0, math.cos(rad)],
    ])
    vertices = xp.dot(vertices, rotation_mat.T)
    return vertices.astype(xp.float32)


def rotate_z(vertices, angle):
    xp = chainer.cuda.get_array_module(vertices)
    rad = math.pi * (angle % 360) / 180.0
    rotation_mat = xp.asarray([
        [math.cos(rad), -math.sin(rad), 0],
        [math.sin(rad), math.cos(rad), 0],
        [0, 0, 1],
    ])
    vertices = xp.dot(vertices, rotation_mat.T)
    return vertices.astype(xp.float32)


# 各面の各頂点番号に対応する座標を取る
def convert_to_face_representation(vertices, faces):
    assert (vertices.ndim == 3)
    assert (faces.ndim == 3)
    assert (vertices.shape[0] == faces.shape[0])
    assert (vertices.shape[2] == 3)
    assert (faces.shape[2] == 3)

    xp = chainer.cuda.get_array_module(faces)
    batch_size, num_vertices = vertices.shape[:2]
    faces = faces + (
        xp.arange(batch_size, dtype=xp.int32) * num_vertices)[:, None, None]
    vertices = vertices.reshape((batch_size * num_vertices, 3)).astype(
        xp.float32)
    return vertices[faces]


# 透視変換
# https://qiita.com/ryutorion/items/0824a8d6f27564e850c9
# ただしこの記事とは違いz \in [0, 1] であり、z_maxとz_minは鏡像変換後のz座標という違いがある
def project_perspective(vertices, viewing_angle, z_max=5, z_min=0, d=1):
    assert (vertices.ndim == 3)
    assert (vertices.shape[2] == 3)

    xp = chainer.cuda.get_array_module(vertices)

    # 鏡像変換と正規化
    vertices *= xp.asarray([[1.0 / z_max, 1.0 / z_max, -1.0 / z_max]])

    z = vertices[..., None, 2]
    z_a = z_max / (z_max - z_min)
    z_b = (z_max * z_min) / (z_min - z_max)
    viewing_rad_half = angle_to_radian(viewing_angle / 2.0)
    projection_mat = xp.asarray([[1.0 / math.tan(viewing_rad_half), 0, 0],
                                 [0.0, 1.0 / math.tan(viewing_rad_half),
                                  0], [0, 0, z_a]])
    vertices = xp.dot(vertices, projection_mat.T)
    vertices += xp.asarray([[0, 0, z_b]])

    return vertices.astype(xp.float32)


# カメラ座標系への変換
# 右手座標系
# カメラから見える範囲にあるオブジェクトのz座標は全て負になる
def transform_to_camera_coordinate_system(vertices, distance_from_object,
                                          angle_x, angle_y):
    assert (vertices.ndim == 3)
    assert (vertices.shape[2] == 3)

    xp = chainer.cuda.get_array_module(vertices)
    rad_x = angle_to_radian(angle_x)
    rad_y = angle_to_radian(angle_y)
    rotation_mat_x = xp.asarray([
        [1, 0, 0],
        [0, math.cos(rad_x), -math.sin(rad_x)],
        [0, math.sin(rad_x), math.cos(rad_x)],
    ])
    rotation_mat_y = xp.asarray([
        [math.cos(rad_y), 0, math.sin(rad_y)],
        [0, 1, 0],
        [-math.sin(rad_y), 0, math.cos(rad_y)],
    ])
    vertices = xp.dot(vertices, rotation_mat_x.T)
    vertices = xp.dot(vertices, rotation_mat_y.T)
    vertices += xp.asarray([[0, 0, -distance_from_object]])
    return vertices.astype(xp.float32)