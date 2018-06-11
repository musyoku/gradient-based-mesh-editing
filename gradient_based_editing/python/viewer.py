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
    window = gme.viewer.Window()
    datasource_gradient = gme.viewer.ImageData(silhouette_size[0],
                                           silhouette_size[1], 1)
    window.add_view(datasource_gradient, 0, 0, 0.5, 0.5)
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

        # print(faces_batch.size)
        # print(face_vertices_batch.size)
        # print(perspective_vertices_batch.size)

        # print(face_index_map_batch.size)
        # print(object_silhouette_batch.size)

        # print(grad_vertices_batch.size)
        # print(grad_silhouette_batch.size)
        # print(debug_grad_map.size)

        # browser.update_top_left_image(np.uint8(grad_image))
        # browser.update_bottom_left_image(np.uint8(debug_grad_map[0]))
        # browser.update_top_right_image(depth_map_image)
        # browser.update_bottom_right_image(
        #     np.uint8(target_silhouette_batch[0]))
        # browser.update_object(np.ascontiguousarray(vertices_batch[0]))
        datasource_gradient.set(depth_map_image)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    args = parser.parse_args()
    main()