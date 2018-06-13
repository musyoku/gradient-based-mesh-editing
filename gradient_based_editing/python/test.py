import os, sys, argparse
import math
import numpy as np
import gradient_based_editing as gme


def run_test(vertices_batch,
             faces_batch,
             target_silhouette_batch,
             axis_sign,
             axis_gradient,
             axis_silhouette,
             axis_target,
             axis_object,
             silhouette_size,
             num_itr=500):
    axis_object.update_vertices(vertices_batch[0])
    for _ in range(num_itr):
        # カメラ座標系に変換
        perspective_vertices_batch = gme.vertices.transform_to_camera_coordinate_system(
            vertices_batch, distance_from_object=2, angle_x=0, angle_y=0)

        # 透視投影
        perspective_vertices_batch = gme.vertices.project_perspective(
            perspective_vertices_batch, viewing_angle=45, z_max=5, z_min=0)

        face_vertices_batch = gme.vertices.convert_to_face_representation(
            perspective_vertices_batch, faces_batch)
        # print(face_vertices_batch.shape)
        batch_size = face_vertices_batch.shape[0]
        face_index_map_batch = np.full(
            (batch_size, ) + silhouette_size, -1, dtype=np.int32)
        depth_map = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.float32)
        object_silhouette_batch = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.int32)
        gme.rasterizer.forward_face_index_map_cpu(
            face_vertices_batch, face_index_map_batch, depth_map,
            object_silhouette_batch)
        depth_map_image = np.ascontiguousarray(
            (1.0 - depth_map[0]) * 255).astype(np.uint8)

        grad_vertices_batch = np.zeros_like(vertices_batch, dtype=np.float32)
        object_silhouette_batch = np.copy(
            (1.0 - depth_map) * 255, order="c").astype(np.int32)
        object_silhouette_batch[object_silhouette_batch > 0] = 255
        grad_silhouette_batch = -(
            (target_silhouette_batch - object_silhouette_batch) / 255).astype(
                np.float32)

        debug_grad_map = np.zeros_like(
            object_silhouette_batch, dtype=np.float32)
        gme.rasterizer.backward_silhouette_cpu(
            faces_batch, face_vertices_batch, perspective_vertices_batch,
            face_index_map_batch, object_silhouette_batch, grad_vertices_batch,
            grad_silhouette_batch, debug_grad_map)

        debug_grad_map = np.abs(debug_grad_map)
        debug_grad_map /= np.amax(debug_grad_map)
        debug_grad_map *= 255

        vertices_batch -= 0.0001 * grad_vertices_batch
        grad_image = np.copy(grad_silhouette_batch[0]) * 255
        grad_image[grad_image > 0] = 255
        grad_image[grad_image < 0] = 64

        axis_sign.update(np.uint8(grad_image))
        axis_silhouette.update(depth_map_image)
        axis_gradient.update(np.uint8(debug_grad_map[0]))
        axis_target.update(np.uint8(target_silhouette_batch[0]))
        axis_object.update_vertices(vertices_batch[0])


def main():
    # オブジェクトの読み込み
    vertices, faces = gme.objects.load("objects/triangle.obj")

    # ミニバッチ化
    original_vertices_batch = vertices[None, ...]
    faces_batch = faces[None, ...]
    batch_size = faces_batch.shape[0]

    silhouette_size = (256, 256)

    figure = gme.viewer.Figure()
    width, height = silhouette_size

    axis_sign = gme.viewer.ImageData(width, height, 1)
    axis_gradient = gme.viewer.ImageData(width, height, 1)
    axis_silhouette = gme.viewer.ImageData(width, height, 1)
    axis_target = gme.viewer.ImageData(width, height, 1)
    axis_object = gme.viewer.ObjectData(vertices, vertices.shape[0], faces,
                                        faces.shape[0])

    figure.add(axis_sign, 0, 0, 0.25, 0.5)
    figure.add(axis_gradient, 0, 0.5, 0.25, 0.5)
    figure.add(axis_silhouette, 0.75, 0, 0.25, 0.5)
    figure.add(axis_target, 0.75, 0.5, 0.25, 0.5)
    figure.add(axis_object, 0.25, 0, 0.5, 1)

    window = gme.viewer.Window(figure)
    window.show()

    if True:
        target_silhouette_batch = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.float32)
        target_silhouette_batch[:, 30:225, 30:225] = 255

        vertices_batch = np.copy(original_vertices_batch)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 90)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 180)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 270)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 60)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 120)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 240)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 300)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

    if True:
        target_silhouette_batch = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.float32)
        target_silhouette_batch[:, 120:140, 40:200] = 255

        vertices_batch = np.copy(original_vertices_batch)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 90)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 180)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 270)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 60)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 120)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 240)
        vertices_batch = np.asarray(
            [[[-1.846143, 0.18859313, 0], [0.6484659, -0.3721265, 0.],
              [1.2827933, 0.32970333, 0.], [0.1804606, -0.8931645, 0.]]],
            dtype=vertices_batch.dtype)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

        vertices_batch = gme.vertices.rotate_z(
            np.copy(original_vertices_batch), 300)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)

    if True:
        target_silhouette_batch = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.float32)
        target_silhouette_batch[:, 30:225, 30:225] = 255

        vertices_batch = np.copy(original_vertices_batch)
        vertices_batch += (1, 0, 0)
        run_test(vertices_batch, faces_batch, target_silhouette_batch,
                 axis_sign, axis_gradient, axis_silhouette, axis_target,
                 axis_object, silhouette_size)


if __name__ == "__main__":
    main()