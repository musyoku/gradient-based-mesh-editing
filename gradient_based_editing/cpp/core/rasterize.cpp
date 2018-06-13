#include "rasterize.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace gme {
float to_projected_coordinate(int p, int size)
{
    return 2.0 * (p / (float)(size - 1) - 0.5);
}

// [-1, 1] -> [0, size - 1]
int to_image_coordinate(float p, int size)
{
    return std::min(std::max((int)std::round((p + 1.0f) * 0.5f * (size - 1)), 0), size - 1);
}

// 各画素ごとに最前面を特定する
void forward_face_index_map(
    py::array_t<float, py::array::c_style> np_face_vertices,
    py::array_t<int, py::array::c_style> np_face_index_map,
    py::array_t<float, py::array::c_style> np_depth_map,
    py::array_t<int, py::array::c_style> np_silhouette_image)
{
    if (np_face_vertices.ndim() != 4) {
        std::runtime_error("(np_face_vertices.ndim() != 4) -> false");
    }
    if (np_depth_map.ndim() != 3) {
        std::runtime_error("(np_depth_map.ndim() != 3) -> false");
    }
    if (np_face_index_map.ndim() != 3) {
        std::runtime_error("(np_face_index_map.ndim() != 3) -> false");
    }
    if (np_silhouette_image.ndim() != 3) {
        std::runtime_error("(np_silhouette_image.ndim() != 3) -> false");
    }


    int batch_size = np_face_vertices.shape(0);
    int num_faces = np_face_vertices.shape(1);
    int image_height = np_silhouette_image.shape(1);
    int image_width = np_silhouette_image.shape(2);

    auto face_vertices = np_face_vertices.mutable_unchecked<4>();
    auto depth_map = np_depth_map.mutable_unchecked<3>();
    auto face_index_map = np_face_index_map.mutable_unchecked<3>();
    auto silhouette_image = np_silhouette_image.mutable_unchecked<3>();

    for (int batch_index = 0; batch_index < batch_size; batch_index++) {
        // 初期化
        for (int yi = 0; yi < image_height; yi++) {
            for (int xi = 0; xi < image_width; xi++) {
                depth_map(batch_index, yi, xi) = 1.0; // 最も遠い位置に初期化
            }
        }
        for (int face_index = 0; face_index < num_faces; face_index++) {
            float xf_1 = face_vertices(batch_index, face_index, 0, 0);
            float yf_1 = face_vertices(batch_index, face_index, 0, 1);
            float zf_1 = face_vertices(batch_index, face_index, 0, 2);
            float xf_2 = face_vertices(batch_index, face_index, 1, 0);
            float yf_2 = face_vertices(batch_index, face_index, 1, 1);
            float zf_2 = face_vertices(batch_index, face_index, 1, 2);
            float xf_3 = face_vertices(batch_index, face_index, 2, 0);
            float yf_3 = face_vertices(batch_index, face_index, 2, 1);
            float zf_3 = face_vertices(batch_index, face_index, 2, 2);

            // カリングによる裏面のスキップ
            // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
            if ((yf_1 - yf_3) * (xf_1 - xf_2) < (yf_1 - yf_2) * (xf_1 - xf_3)) {
                continue;
            }

            // 全画素についてループ
            for (int yi = 0; yi < image_height; yi++) {
                // yi \in [0, image_height] -> yf \in [-1, 1]
                float yf = -to_projected_coordinate(yi, image_height);
                // y座標が面の外部ならスキップ
                if ((yf > yf_1 && yf > yf_2 && yf > yf_3) || (yf < yf_1 && yf < yf_2 && yf < yf_3)) {
                    continue;
                }
                for (int xi = 0; xi < image_width; xi++) {
                    // xi \in [0, image_width] -> xf \in [-1, 1]
                    float xf = to_projected_coordinate(xi, image_width);

                    // xyが面の外部ならスキップ
                    // Edge Functionで3辺のいずれかの右側にあればスキップ
                    // https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
                    if ((yf - yf_1) * (xf_2 - xf_1) < (xf - xf_1) * (yf_2 - yf_1) || (yf - yf_2) * (xf_3 - xf_2) < (xf - xf_2) * (yf_3 - yf_2) || (yf - yf_3) * (xf_1 - xf_3) < (xf - xf_3) * (yf_1 - yf_3)) {
                        continue;
                    }

                    // 重心座標系の各係数を計算
                    // http://zellij.hatenablog.com/entry/20131207/p1
                    float lambda_1 = ((yf_2 - yf_3) * (xf - xf_3) + (xf_3 - xf_2) * (yf - yf_3)) / ((yf_2 - yf_3) * (xf_1 - xf_3) + (xf_3 - xf_2) * (yf_1 - yf_3));
                    float lambda_2 = ((yf_3 - yf_1) * (xf - xf_3) + (xf_1 - xf_3) * (yf - yf_3)) / ((yf_2 - yf_3) * (xf_1 - xf_3) + (xf_3 - xf_2) * (yf_1 - yf_3));
                    float lambda_3 = 1.0 - lambda_1 - lambda_2;

                    // 面f_nのxy座標に対応する点のz座標を求める
                    // https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation
                    float z_face = 1.0 / (lambda_1 / zf_1 + lambda_2 / zf_2 + lambda_3 / zf_3);

                    if (z_face < 0.0 || z_face > 1.0) {
                        continue;
                    }
                    // zは小さい方が手前
                    float current_min_z = depth_map(batch_index, yi, xi);
                    if (z_face < current_min_z) {
                        // 現在の面の方が前面の場合
                        depth_map(batch_index, yi, xi) = z_face;
                        face_index_map(batch_index, yi, xi) = face_index;
                        silhouette_image(batch_index, yi, xi) = 255;
                    }
                }
            }
        }
    }
}

void compute_grad_y(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    float xf_c,
    float yf_c,
    int vertex_index_a,
    int vertex_index_b,
    int image_width,
    int image_height,
    int num_vertices,
    int target_batch_index,
    int target_face_index,
    py::array_t<int, py::array::c_style>& np_face_index_map,
    py::array_t<int, py::array::c_style>& np_pixel_map,
    py::array_t<float, py::array::c_style>& np_grad_vertices,
    py::array_t<float, py::array::c_style>& np_grad_silhouette,
    py::array_t<float, py::array::c_style>& np_debug_grad_map)
{
    auto face_index_map = np_face_index_map.mutable_unchecked<3>();
    auto pixel_map = np_pixel_map.mutable_unchecked<3>();
    auto grad_vertices = np_grad_vertices.mutable_unchecked<3>();
    auto grad_silhouette = np_grad_silhouette.mutable_unchecked<3>();
    auto debug_grad_map = np_debug_grad_map.mutable_unchecked<3>();

    // 画像座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
    // 画像配列に合わせるためそのような座標系になる
    int xi_a = to_image_coordinate(xf_a, image_width);
    int xi_b = to_image_coordinate(xf_b, image_width);
    int xi_c = to_image_coordinate(xf_c, image_width);
    // int yi_a = (image_height - 1) - to_image_coordinate(yf_a, image_height);
    // int yi_b = (image_height - 1) - to_image_coordinate(yf_b, image_height);
    // int yi_c = (image_height - 1) - to_image_coordinate(yf_c, image_height);

    // if (xi_a == xi_b) {
    //     return;
    // }

    // y方向の走査がどの方向を向いていると辺に当たるか
    // 1:  画像の上から下に進む（yが増加する）方向に進んだ時に辺に当たる
    // -1: 画像の下から上に進む（yが減少する）方向に進んだ時に辺に当たる
    int top_to_bottom = 1;
    int bottom_to_top = -1;
    int scan_direction = (xi_a < xi_b) ? bottom_to_top : top_to_bottom;

    // 辺に沿ってx軸を走査
    int xi_p_start = std::min(xi_a, xi_b);
    int xi_p_end = std::max(xi_a, xi_b);
    // 辺上でx座標がpi_xの点を求める
    // 論文の図の点I_ijに相当（ここでは交点と呼ぶ）
    for (int xi_p = xi_p_start; xi_p <= xi_p_end; xi_p++) {
        // 辺に当たるまでy軸を走査
        // ここではスキャンラインと呼ぶことにする
        if (scan_direction == top_to_bottom) {
            // まずスキャンライン上で辺に当たる画素を探す
            int yi_s_start = 0;
            int yi_s_end = image_height - 1; // 実際にはここに到達する前に辺に当たるはず
            int yi_s_edge = yi_s_start;
            // 最初から面の内部の場合はスキップ
            int face_index = face_index_map(target_batch_index, yi_s_start, xi_p);
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int yi_s = yi_s_start; yi_s <= yi_s_end; yi_s++) {
                    int face_index = face_index_map(target_batch_index, yi_s, xi_p);
                    if (face_index == target_face_index) {
                        yi_s_edge = yi_s;
                        pixel_value_inside = pixel_map(target_batch_index, yi_s, xi_p);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int yi_s = yi_s_start; yi_s < yi_s_edge; yi_s++) {
                    int pixel_value_outside = pixel_map(target_batch_index, yi_s, xi_p);
                    // 走査点と面の輝度値の差
                    float delta_ij = pixel_value_inside - pixel_value_outside;
                    float delta_pj = grad_silhouette(target_batch_index, yi_s, xi_p);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 頂点の実際の移動量を求める
                    // スキャンライン上の移動距離ではない
                    // 相似な三角形なのでx方向の比率から求まる
                    // 頂点Aについて
                    {
                        if (xi_p - xi_p_start > 0) {
                            float moving_distance = (yi_s_edge - yi_s) / (float)(xi_p - xi_p_start) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                    // 頂点Bについて
                    {
                        if (xi_p_end - xi_p > 0) {
                            float moving_distance = (yi_s_edge - yi_s) / (float)(xi_p_end - xi_p) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int pixel_value_outside = pixel_map(target_batch_index, (yi_s_edge - 1), xi_p);
                int yi_s_other_edge = image_height - 1;
                int pixel_value_other_outside = 0;
                bool edge_found = false;
                // 反対側の辺の位置を特定する
                for (int yi_s = yi_s_edge + 1; yi_s <= yi_s_end; yi_s++) {
                    int face_index = face_index_map(target_batch_index, yi_s, xi_p);
                    if (face_index != target_face_index) {
                        yi_s_other_edge = yi_s - 1;
                        pixel_value_other_outside = pixel_map(target_batch_index, yi_s, xi_p);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int yi_s = yi_s_edge; yi_s <= yi_s_other_edge; yi_s++) {
                    int pixel_value_inside = pixel_map(target_batch_index, yi_s, xi_p);
                    float delta_pj = grad_silhouette(target_batch_index, yi_s, xi_p);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 面の上側（頂点を下に動かしていって走査点が辺に当たる場合）
                    {
                        float delta_ij = pixel_value_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点の実際の移動量を求める
                        // スキャンライン上の移動距離ではない
                        // 相似な三角形なのでy方向の比率から求まる
                        if (xi_p - xi_p_start > 0) {
                            float moving_distance = (yi_s - yi_s_edge) / (float)(xi_p - xi_p_start) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                        // 頂点Bについて
                        // 頂点の実際の移動量を求める
                        // スキャンライン上の移動距離ではない
                        // 相似な三角形なのでy方向の比率から求まる
                        if (xi_p_end - xi_p > 0) {
                            float moving_distance = (yi_s - yi_s_edge) / (float)(xi_p_end - xi_p) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }

                    // 面の下側（頂点を上に動かしていって走査点が辺に当たる場合）
                    {
                        float delta_ij = pixel_value_other_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点Cの位置によっては頂点Aをどれだけ移動させても辺が走査点に当たらないことがある
                        if (xi_p > xi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s_other_edge - yi_s) / (float)(xi_p - xi_c) * (float)(xi_p_end - xi_c);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (xi_p < xi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s_other_edge - yi_s) / (float)(xi_c - xi_p) * (float)(xi_c - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                }
            }
        } else {
            int yi_s_start = image_height - 1;
            int yi_s_end = 0;
            int yi_s_edge = yi_s_start;
            // 最初から面の内部の場合はスキップ
            int face_index = face_index_map(target_batch_index, yi_s_start, xi_p);
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int yi_s = yi_s_start; yi_s >= yi_s_end; yi_s--) {
                    int face_index = face_index_map(target_batch_index, yi_s, xi_p);
                    if (face_index == target_face_index) {
                        yi_s_edge = yi_s;
                        pixel_value_inside = pixel_map(target_batch_index, yi_s, xi_p);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int yi_s = yi_s_start; yi_s > yi_s_edge; yi_s--) {
                    int pixel_value_outside = pixel_map(target_batch_index, yi_s, xi_p);
                    float delta_ij = pixel_value_inside - pixel_value_outside;
                    float delta_pj = grad_silhouette(target_batch_index, yi_s, xi_p);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 頂点Aについて
                    {
                        if (xi_p - xi_p_start > 0) {
                            float moving_distance = (yi_s - yi_s_edge) / (float)(xi_p - xi_p_start) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                    // 頂点Bについて
                    {
                        if (xi_p_end - xi_p > 0) {
                            float moving_distance = (yi_s - yi_s_edge) / (float)(xi_p_end - xi_p) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int pixel_value_outside = pixel_map(target_batch_index, (yi_s_edge + 1), xi_p);
                int yi_s_other_edge = 0;
                int pixel_value_other_outside = 0;
                bool edge_found = false;
                // 反対側の辺の位置を特定する
                for (int yi_s = yi_s_edge - 1; yi_s >= yi_s_end; yi_s--) {
                    int face_index = face_index_map(target_batch_index, yi_s, xi_p);
                    if (face_index != target_face_index) {
                        yi_s_other_edge = yi_s + 1;
                        pixel_value_other_outside = pixel_map(target_batch_index, yi_s, xi_p);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int yi_s = yi_s_edge; yi_s >= yi_s_other_edge; yi_s--) {
                    int pixel_value_inside = pixel_map(target_batch_index, yi_s, xi_p);
                    float delta_pj = grad_silhouette(target_batch_index, yi_s, xi_p);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 面の上側（頂点を下に動かしていって走査点が辺に当たる場合）
                    {
                        float delta_ij = pixel_value_other_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点Cの位置によっては頂点Aをどれだけ移動させても辺が走査点に当たらないことがある
                        if (xi_p < xi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s - yi_s_other_edge) / (float)(xi_c - xi_p) * (float)(xi_c - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (xi_p > xi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s - yi_s_other_edge) / (float)(xi_p - xi_c) * (float)(xi_p_end - xi_c);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                    // 面の下側（頂点を上に動かしていって走査点が辺に当たる場合）
                    {
                        float delta_ij = pixel_value_outside - pixel_value_inside;
                        // 頂点Aについて
                        if (xi_p_end - xi_p > 0) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s_edge - yi_s) / (float)(xi_p_end - xi_p) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (xi_p - xi_p_start) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s_edge - yi_s) / (float)(xi_p - xi_p_start) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 1) += grad;
                                debug_grad_map(target_batch_index, yi_s, xi_p) += grad;
                            }
                        }
                    }
                }
            }
        }
    }
    // y方向の各画素を走査
}

void compute_grad_x(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    float xf_c,
    float yf_c,
    int vertex_index_a,
    int vertex_index_b,
    int image_width,
    int image_height,
    int num_vertices,
    int target_batch_index,
    int target_face_index,
    py::array_t<int, py::array::c_style>& np_face_index_map,
    py::array_t<int, py::array::c_style>& np_pixel_map,
    py::array_t<float, py::array::c_style>& np_grad_vertices,
    py::array_t<float, py::array::c_style>& np_grad_silhouette,
    py::array_t<float, py::array::c_style>& np_debug_grad_map)
{
    auto face_index_map = np_face_index_map.mutable_unchecked<3>();
    auto pixel_map = np_pixel_map.mutable_unchecked<3>();
    auto grad_vertices = np_grad_vertices.mutable_unchecked<3>();
    auto grad_silhouette = np_grad_silhouette.mutable_unchecked<3>();
    auto debug_grad_map = np_debug_grad_map.mutable_unchecked<3>();

    // 画像座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
    // 画像配列に合わせるためそのような座標系になる
    // int xi_a = to_image_coordinate(xf_a, image_width);
    // int xi_b = to_image_coordinate(xf_b, image_width);
    // int xi_c = to_image_coordinate(xf_c, image_width);
    int yi_a = (image_height - 1) - to_image_coordinate(yf_a, image_height);
    int yi_b = (image_height - 1) - to_image_coordinate(yf_b, image_height);
    int yi_c = (image_height - 1) - to_image_coordinate(yf_c, image_height);

    // if (yi_a == yi_b) {
    //     return;
    // }

    // x方向の走査がどの方向を向いていると辺に当たるか
    // 1:  画像の左から右に進む（xが増加する）方向に進んだ時に辺に当たる
    // -1: 画像の右から左に進む（xが減少する）方向に進んだ時に辺に当たる
    int left_to_right = 1;
    int right_to_left = -1;
    int scan_direction = (yi_a < yi_b) ? left_to_right : right_to_left;

    int yi_p_start = std::min(yi_a, yi_b);
    int yi_p_end = std::max(yi_a, yi_b);

    // 辺に沿ってy軸を走査
    // 辺上でy座標がyi_pの点を求める
    // 論文の図の点I_ijに相当（ここでは交点と呼ぶ）
    for (int yi_p = yi_p_start; yi_p <= yi_p_end; yi_p++) {
        // 辺に当たるまでx軸を走査
        // ここではスキャンラインと呼ぶことにする
        if (scan_direction == left_to_right) {
            // まずスキャンライン上で辺に当たる画素を探す
            int si_x_start = 0;
            int si_x_end = image_width - 1; // 実際にはここに到達する前に辺に当たるはず
            int xi_s_edge = si_x_start;
            // 最初から面の内部の場合はスキップ
            int face_index = face_index_map(target_batch_index, yi_p, si_x_start);
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int xi_s = si_x_start; xi_s <= si_x_end; xi_s++) {
                    int face_index = face_index_map(target_batch_index, yi_p, xi_s);
                    if (face_index == target_face_index) {
                        xi_s_edge = xi_s;
                        pixel_value_inside = pixel_map(target_batch_index, yi_p, xi_s);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int xi_s = si_x_start; xi_s < xi_s_edge; xi_s++) {
                    int pixel_value_outside = pixel_map(target_batch_index, yi_p, xi_s);
                    float delta_pj = grad_silhouette(target_batch_index, yi_p, xi_s);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 走査点と面の輝度値の差
                    float delta_ij = pixel_value_inside - pixel_value_outside;
                    // 頂点の実際の移動量を求める
                    // スキャンライン上の移動距離ではない
                    // 相似な三角形なのでy方向の比率から求まる
                    // 頂点Aについて
                    if (yi_p_end - yi_p > 0) {
                        float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                        if (moving_distance > 0) {
                            // 左側は勾配が逆向きになる
                            float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                            grad_vertices(target_batch_index, vertex_index_a, 0) += grad;
                            debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                        }
                    }
                    // 頂点Bについて
                    if (yi_p - yi_p_start > 0) {
                        float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                        if (moving_distance > 0) {
                            // 左側は勾配が逆向きになる
                            float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                            grad_vertices(target_batch_index, vertex_index_b, 0) += grad;
                            debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int pixel_value_outside = pixel_map(target_batch_index, yi_p, xi_s_edge - 1);
                int xi_s_other_edge = image_width - 1;
                int pixel_value_other_outside = 0;
                bool edge_found = false;
                // 反対側の辺の位置を特定する
                for (int xi_s = xi_s_edge + 1; xi_s <= si_x_end; xi_s++) {
                    int face_index = face_index_map(target_batch_index, yi_p, xi_s);
                    if (face_index != target_face_index) {
                        xi_s_other_edge = xi_s - 1;
                        pixel_value_other_outside = pixel_map(target_batch_index, yi_p, xi_s);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int xi_s = xi_s_edge; xi_s <= xi_s_other_edge; xi_s++) {
                    int pixel_value_inside = pixel_map(target_batch_index, yi_p, xi_s);
                    float delta_pj = grad_silhouette(target_batch_index, yi_p, xi_s);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 面の左側（頂点を右に動かしていって辺に当たる場合）
                    // 論文のdelta_ij_bに対応
                    {
                        float delta_ij = pixel_value_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点の実際の移動量を求める
                        // スキャンライン上の移動距離ではない
                        // 相似な三角形なのでy方向の比率から求まる
                        if (yi_p - yi_p_start > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                        // 頂点Bについて
                        // 頂点の実際の移動量を求める
                        // スキャンライン上の移動距離ではない
                        // 相似な三角形なのでy方向の比率から求まる
                        if (yi_p_end - yi_p > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                    }

                    // 面の右側（頂点を左に動かしていって辺に当たる場合）
                    // 論文のdelta_ij_aに対応
                    {
                        float delta_ij = pixel_value_other_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点Cの位置によっては頂点Aをどれだけ移動させても辺が走査点に当たらないことがある
                        if (yi_p < yi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s_other_edge - xi_s) / (float)(yi_c - yi_p) * (float)(yi_c - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p > yi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s_other_edge - xi_s) / (float)(yi_p - yi_c) * (float)(yi_p_end - yi_c);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                    }
                }
            }
        } else {
            int si_x_start = image_width - 1;
            int si_x_end = 0;
            int xi_s_edge = si_x_start;
            // 最初から面の内部の場合はスキップ
            int face_index = face_index_map(target_batch_index, yi_p, si_x_start);
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int xi_s = si_x_start; xi_s >= si_x_end; xi_s--) {
                    int face_index = face_index_map(target_batch_index, yi_p, xi_s);
                    if (face_index == target_face_index) {
                        xi_s_edge = xi_s;
                        pixel_value_inside = pixel_map(target_batch_index, yi_p, xi_s);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                if (xi_s_edge < si_x_start) {
                    for (int xi_s = si_x_start; xi_s > xi_s_edge; xi_s--) {
                        int pixel_value_outside = pixel_map(target_batch_index, yi_p, xi_s);
                        float delta_ij = pixel_value_inside - pixel_value_outside;
                        float delta_pj = grad_silhouette(target_batch_index, yi_p, xi_s);
                        if (delta_pj == 0) {
                            continue;
                        }

                        // 頂点Aについて
                        if (yi_p - yi_p_start > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p_end - yi_p > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int pixel_value_outside = pixel_map(target_batch_index, yi_p, xi_s_edge + 1);
                int xi_s_other_edge = 0;
                int pixel_value_other_outside = 0;
                bool edge_found = false;
                // 反対側の辺の位置を特定する
                for (int xi_s = xi_s_edge - 1; xi_s >= si_x_end; xi_s--) {
                    int face_index = face_index_map(target_batch_index, yi_p, xi_s);
                    if (face_index != target_face_index) {
                        xi_s_other_edge = xi_s + 1;
                        pixel_value_other_outside = pixel_map(target_batch_index, yi_p, xi_s);
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int xi_s = xi_s_edge; xi_s >= xi_s_other_edge; xi_s--) {
                    int pixel_value_inside = pixel_map(target_batch_index, yi_p, xi_s);
                    float delta_pj = grad_silhouette(target_batch_index, yi_p, xi_s);
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 面の左側（頂点を右に動かしていって走査点が辺に当たる場合）
                    // 論文のdelta_ij_bに対応
                    {
                        float delta_ij = pixel_value_other_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点Cの位置によっては頂点Aをどれだけ移動させても辺が走査点に当たらないことがある
                        if (yi_p > yi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s - xi_s_other_edge) / (float)(yi_p - yi_c) * (float)(yi_p_end - yi_c);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p < yi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s - xi_s_other_edge) / (float)(yi_p - yi_c) * (float)(yi_c - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                    }
                    // 面の右側（頂点を左に動かしていって辺に当たる場合）
                    // 論文のdelta_ij_aに対応
                    {
                        float delta_ij = pixel_value_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点Cの位置によっては頂点Aをどれだけ移動させても辺が走査点に当たらないことがある
                        if (yi_p - yi_p_start > 0) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_a, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p_end - yi_p) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices(target_batch_index, vertex_index_b, 0) += grad;
                                debug_grad_map(target_batch_index, yi_p, xi_s) += grad;
                            }
                        }
                    }
                }
            }
        }
    }
    // y方向の各画素を走査
}

// 点ABからなる辺の外側と内側の画素を網羅して勾配を計算する
// *f_* \in [-1, 1]
// xi_* \in [0, image_width - 1]
// yi_* \in [0, image_height - 1]
void compute_grad(
    float xf_a,
    float yf_a,
    float xf_b,
    float yf_b,
    float xf_c,
    float yf_c,
    int vertex_index_a,
    int vertex_index_b,
    int image_width,
    int image_height,
    int num_vertices,
    int target_batch_index,
    int target_face_index,
    py::array_t<int, py::array::c_style>& np_face_index_map,
    py::array_t<int, py::array::c_style>& np_pixel_map,
    py::array_t<float, py::array::c_style>& np_grad_vertices,
    py::array_t<float, py::array::c_style>& np_grad_silhouette,
    py::array_t<float, py::array::c_style>& np_debug_grad_map)
{
    compute_grad_x(
        xf_a,
        yf_a,
        xf_b,
        yf_b,
        xf_c,
        yf_c,
        vertex_index_a,
        vertex_index_b,
        image_width,
        image_height,
        num_vertices,
        target_batch_index,
        target_face_index,
        np_face_index_map,
        np_pixel_map,
        np_grad_vertices,
        np_grad_silhouette,
        np_debug_grad_map);
    compute_grad_y(
        xf_a,
        yf_a,
        xf_b,
        yf_b,
        xf_c,
        yf_c,
        vertex_index_a,
        vertex_index_b,
        image_width,
        image_height,
        num_vertices,
        target_batch_index,
        target_face_index,
        np_face_index_map,
        np_pixel_map,
        np_grad_vertices,
        np_grad_silhouette,
        np_debug_grad_map);
}

// シルエットの誤差から各頂点の勾配を求める
void backward_silhouette(
    py::array_t<int, py::array::c_style> np_faces,
    py::array_t<float, py::array::c_style> np_face_vertices,
    py::array_t<float, py::array::c_style> np_vertices,
    py::array_t<int, py::array::c_style> np_face_index_map,
    py::array_t<int, py::array::c_style> np_pixel_map,
    py::array_t<float, py::array::c_style> np_grad_vertices,
    py::array_t<float, py::array::c_style> np_grad_silhouette,
    py::array_t<float, py::array::c_style> np_debug_grad_map)
{
    if (np_faces.ndim() != 3) {
        std::runtime_error("(np_faces.ndim() != 3) -> false");
    }
    if (np_face_vertices.ndim() != 4) {
        std::runtime_error("(np_face_vertices.ndim() != 4) -> false");
    }
    if (np_vertices.ndim() != 3) {
        std::runtime_error("(np_vertices.ndim() != 3) -> false");
    }
    if (np_face_index_map.ndim() != 3) {
        std::runtime_error("(np_face_index_map.ndim() != 3) -> false");
    }
    if (np_pixel_map.ndim() != 3) {
        std::runtime_error("(np_pixel_map.ndim() != 3) -> false");
    }
    if (np_grad_vertices.ndim() != 3) {
        std::runtime_error("(np_grad_vertices.ndim() != 3) -> false");
    }
    if (np_grad_silhouette.ndim() != 3) {
        std::runtime_error("(np_grad_silhouette.ndim() != 3) -> false");
    }
    if (np_debug_grad_map.ndim() != 3) {
        std::runtime_error("(np_debug_grad_map.ndim() != 3) -> false");
    }

    int batch_size = np_face_vertices.shape(0);
    int num_faces = np_face_vertices.shape(1);
    int num_vertices = np_vertices.shape(1);
    int image_height = np_face_index_map.shape(1);
    int image_width = np_face_index_map.shape(2);

    auto face_vertices = np_face_vertices.mutable_unchecked<4>();
    auto faces = np_faces.mutable_unchecked<3>();

    for (int batch_index = 0; batch_index < batch_size; batch_index++) {
        for (int face_index = 0; face_index < num_faces; face_index++) {
            float xf_1 = face_vertices(batch_index, face_index, 0, 0);
            float yf_1 = face_vertices(batch_index, face_index, 0, 1);
            // float zf_1 = face_vertices(batch_index, face_index, 0, 2);
            float xf_2 = face_vertices(batch_index, face_index, 1, 0);
            float yf_2 = face_vertices(batch_index, face_index, 1, 1);
            // float zf_2 = face_vertices(batch_index, face_index, 0, 2);
            float xf_3 = face_vertices(batch_index, face_index, 2, 0);
            float yf_3 = face_vertices(batch_index, face_index, 2, 1);
            // float zf_3 = face_vertices(batch_index, face_index, 0, 2);

            int vertex_1_index = faces(batch_index, face_index, 0);
            int vertex_2_index = faces(batch_index, face_index, 1);
            int vertex_3_index = faces(batch_index, face_index, 2);

            // カリングによる裏面のスキップ
            // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
            if ((yf_1 - yf_3) * (xf_1 - xf_2) < (yf_1 - yf_2) * (xf_1 - xf_3)) {
                continue;
            }

            // 3辺について
            compute_grad(
                xf_1,
                yf_1,
                xf_2,
                yf_2,
                xf_3,
                yf_3,
                vertex_1_index,
                vertex_2_index,
                image_width,
                image_height,
                num_vertices,
                batch_index,
                face_index,
                np_face_index_map,
                np_pixel_map,
                np_grad_vertices,
                np_grad_silhouette,
                np_debug_grad_map);
            compute_grad(
                xf_2,
                yf_2,
                xf_3,
                yf_3,
                xf_1,
                yf_1,
                vertex_2_index,
                vertex_3_index,
                image_width,
                image_height,
                num_vertices,
                batch_index,
                face_index,
                np_face_index_map,
                np_pixel_map,
                np_grad_vertices,
                np_grad_silhouette,
                np_debug_grad_map);
            compute_grad(
                xf_3,
                yf_3,
                xf_1,
                yf_1,
                xf_2,
                yf_2,
                vertex_3_index,
                vertex_1_index,
                image_width,
                image_height,
                num_vertices,
                batch_index,
                face_index,
                np_face_index_map,
                np_pixel_map,
                np_grad_vertices,
                np_grad_silhouette,
                np_debug_grad_map);
        }
    }
}
}