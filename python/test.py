import os, sys, argparse
import math
import numpy as np
import gradient_based_editing as gm


def run_test(vertices_batch, faces_batch, browser, silhouette_size):
    browser.init_object(
        np.ascontiguousarray(vertices_batch[0]),
        np.ascontiguousarray(faces_batch[0]))
    for _ in range(200):
        # カメラ座標系に変換
        perspective_vertices_batch = gm.vertices.transform_to_camera_coordinate_system(
            vertices_batch, distance_from_object=2, angle_x=0, angle_y=0)

        # 透視投影
        perspective_vertices_batch = gm.vertices.project_perspective(
            perspective_vertices_batch, viewing_angle=45, z_max=5, z_min=0)

        face_vertices_batch = gm.vertices.convert_to_face_representation(
            perspective_vertices_batch, faces_batch)
        # print(face_vertices_batch.shape)
        batch_size = face_vertices_batch.shape[0]
        face_index_map_batch = np.full(
            (batch_size, ) + silhouette_size, -1, dtype=np.int32)
        depth_map = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.float32)
        object_silhouette_batch = np.zeros(
            (batch_size, ) + silhouette_size, dtype=np.int32)
        gm.rasterizer.forward_face_index_map_cpu(
            face_vertices_batch, face_index_map_batch, depth_map,
            object_silhouette_batch)
        depth_map_image = np.ascontiguousarray(
            (1.0 - depth_map[0]) * 255).astype(np.uint8)

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
        gm.rasterizer.backward_silhouette_cpu(
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

        browser.update_top_left_image(np.uint8(grad_image))
        browser.update_bottom_left_image(np.uint8(debug_grad_map[0]))
        browser.update_top_right_image(depth_map_image)
        browser.update_bottom_right_image(np.uint8(target_silhouette_batch[0]))
        browser.update_object(np.ascontiguousarray(vertices_batch[0]))


def main():
    # オブジェクトの読み込み
    vertices, faces = gm.objects.load("objects/triangle.obj")

    # ミニバッチ化
    original_vertices_batch = vertices[None, ...]
    faces_batch = faces[None, ...]

    silhouette_size = (256, 256)

    # ブラウザのビューワを起動
    # nodeのサーバーをあらかじめ起動しておかないと繋がらないので注意
    browser = gm.browser.Silhouette(
        8080, np.ascontiguousarray(original_vertices_batch[0]),
        np.ascontiguousarray(faces_batch[0]), silhouette_size)

    if True:
        vertices_batch = np.copy(original_vertices_batch)
        run_test(vertices_batch, faces_batch, browser, silhouette_size)

        vertices_batch = gm.vertices.rotate_z(np.copy(original_vertices_batch), 90)
        run_test(vertices_batch, faces_batch, browser, silhouette_size)

        vertices_batch = gm.vertices.rotate_z(
            np.copy(original_vertices_batch), 180)
        run_test(vertices_batch, faces_batch, browser, silhouette_size)

        vertices_batch = gm.vertices.rotate_z(
            np.copy(original_vertices_batch), 270)
        run_test(vertices_batch, faces_batch, browser, silhouette_size)
    
    if False:
        vertices_batch = np.copy(original_vertices_batch)
        vertices_batch += (1, 0, 0)
        run_test(vertices_batch, faces_batch, browser, silhouette_size)


if __name__ == "__main__":
    main()