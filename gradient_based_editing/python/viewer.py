import os, sys, argparse
import math
import numpy as np
import gradient_based_editing as gme


def main():
    # オブジェクトの読み込み
    vertices, faces = gme.objects.load("objects/teapot.obj")

    # ミニバッチ化
    vertices_batch = vertices[None, ...]
    faces_batch = faces[None, ...]

    silhouette_size = (256, 256)

    figure = gme.viewer.Figure()
    width, height = silhouette_size

    axis_sign = gme.viewer.ImageData(width, height, 1)
    axis_gradient = gme.viewer.ImageData(width, height, 1)
    axis_silhouette = gme.viewer.ImageData(width, height, 1)
    axis_target = gme.viewer.ImageData(width, height, 1)
    axis_object = gme.viewer.ObjectData(vertices, vertices.shape[0], faces, faces.shape[0])

    figure.add(axis_sign, 0, 0, 0.25, 0.5)
    figure.add(axis_gradient, 0, 0.5, 0.25, 0.5)
    figure.add(axis_silhouette, 0.75, 0, 0.25, 0.5)
    figure.add(axis_target, 0.75, 0.5, 0.25, 0.5)
    figure.add(axis_object, 0.25, 0, 0.5, 1)

    window = gme.viewer.Window(figure)
    window.show()

    # オブジェクトを適当に回転させる
    angle_x = np.random.randint(-180, 180)
    angle_y = np.random.randint(-180, 180)
    angle_z = np.random.randint(-180, 180)
    angle_x = -30
    angle_y = -45
    angle_z = 0
    vertices_batch = gme.vertices.rotate_x(vertices_batch, angle_x)
    vertices_batch = gme.vertices.rotate_y(vertices_batch, angle_y)
    vertices_batch = gme.vertices.rotate_z(vertices_batch, angle_z)

    for _ in range(10000):
        # カメラ座標系に変換
        perspective_vertices_batch = gme.vertices.transform_to_camera_coordinate_system(
            vertices_batch, distance_from_object=2, angle_x=0, angle_y=0)

        # 透視投影
        perspective_vertices_batch = gme.vertices.project_perspective(
            perspective_vertices_batch, viewing_angle=45, z_max=5, z_min=0)

        #################
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
        #################

        #################
        target_silhouette_batch = np.zeros_like(
            object_silhouette_batch, dtype=np.float32)
        target_silhouette_batch[:, 30:225, 30:225] = 255
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

        vertices_batch -= 0.00005 * grad_vertices_batch
        grad_image = np.copy(grad_silhouette_batch[0]) * 255
        grad_image[grad_image > 0] = 255
        grad_image[grad_image < 0] = 64
        #################

        # browser.update_object(np.ascontiguousarray(vertices_batch[0]))
        axis_sign.update(np.uint8(grad_image))
        axis_silhouette.update(depth_map_image)
        axis_gradient.update(np.uint8(debug_grad_map[0]))
        axis_target.update(np.uint8(target_silhouette_batch[0]))
        axis_object.update_vertices(vertices_batch[0])

        if window.closed():
            return


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()
    main()