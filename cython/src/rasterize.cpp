#include "rasterize.h"
#include <cmath>
#include <iostream>

using std::cout;
using std::endl;

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
void cpp_forward_face_index_map(
    float* face_vertices,
    int* face_index_map,
    float* depth_map,
    int* silhouette_image,
    int batch_size,
    int num_faces,
    int image_width,
    int image_height)
{
    for (int batch_index = 0; batch_index < batch_size; batch_index++) {
        // 初期化
        for (int yi = 0; yi < image_height; yi++) {
            for (int xi = 0; xi < image_width; xi++) {
                int index = batch_index * image_width * image_height + yi * image_width + xi;
                depth_map[index] = 1.0; // 最も遠い位置に初期化
            }
        }
        for (int face_index = 0; face_index < num_faces; face_index++) {
            int fv_index = batch_index * num_faces * 9 + face_index * 9;
            float xf_1 = face_vertices[fv_index + 0];
            float yf_1 = face_vertices[fv_index + 1];
            float zf_1 = face_vertices[fv_index + 2];
            float xf_2 = face_vertices[fv_index + 3];
            float yf_2 = face_vertices[fv_index + 4];
            float zf_2 = face_vertices[fv_index + 5];
            float xf_3 = face_vertices[fv_index + 6];
            float yf_3 = face_vertices[fv_index + 7];
            float zf_3 = face_vertices[fv_index + 8];

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
                    // xi \in [0, image_height] -> xf \in [-1, 1]
                    float xf = to_projected_coordinate(xi, image_width);

                    // xyが面の外部ならスキップ
                    // Edge Functionで3辺のいずれかの右側にあればスキップ
                    // https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
                    if ((yf - yf_1) * (xf_2 - xf_1) < (xf - xf_1) * (yf_2 - yf_1) || (yf - yf_2) * (xf_3 - xf_2) < (xf - xf_2) * (yf_3 - yf_2) || (yf - yf_3) * (xf_1 - xf_3) < (xf - xf_3) * (yf_1 - yf_3)) {
                        continue;
                    }

                    int map_index = batch_index * image_height * image_width + yi * image_width + xi;

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
                    float current_min_z = depth_map[map_index];
                    if (z_face < current_min_z) {
                        // 現在の面の方が前面の場合
                        depth_map[map_index] = z_face;
                        face_index_map[map_index] = face_index;
                        silhouette_image[map_index] = 255;
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
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map)
{
    // 画像座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
    // 画像配列に合わせるためそのような座標系になる
    int xi_a = to_image_coordinate(xf_a, image_width);
    int xi_b = to_image_coordinate(xf_b, image_width);
    int xi_c = to_image_coordinate(xf_c, image_width);
    int yi_a = (image_height - 1) - to_image_coordinate(yf_a, image_height);
    int yi_b = (image_height - 1) - to_image_coordinate(yf_b, image_height);
    int yi_c = (image_height - 1) - to_image_coordinate(yf_c, image_height);

    if (xi_a == xi_b) {
        return;
    }

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
            int map_index = target_batch_index * image_width * image_height + yi_s_start * image_width + xi_p;
            int face_index = face_index_map[map_index];
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int yi_s = yi_s_start; yi_s <= yi_s_end; yi_s++) {
                    int map_index = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int face_index = face_index_map[map_index];
                    if (face_index == target_face_index) {
                        yi_s_edge = yi_s;
                        pixel_value_inside = pixel_map[map_index];
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int yi_s = yi_s_start; yi_s < yi_s_edge; yi_s++) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int pixel_value_outside = pixel_map[map_index_s];
                    // 走査点と面の輝度値の差
                    float delta_ij = pixel_value_inside - pixel_value_outside;
                    float delta_pj = grad_silhouette[map_index_s];
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                    }
                    // 頂点Bについて
                    {
                        if (xi_p_end - xi_p > 0) {
                            float moving_distance = (yi_s_edge - yi_s) / (float)(xi_p_end - xi_p) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int map_index_outside = target_batch_index * image_width * image_height + (yi_s_edge - 1) * image_width + xi_p;
                int pixel_value_outside = pixel_map[map_index_outside];
                int yi_s_other_edge = image_height - 1;
                int pixel_value_other_outside = 0;
                // 反対側の辺の位置を特定する
                for (int yi_s = yi_s_edge + 1; yi_s <= yi_s_end; yi_s++) {
                    int map_index = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int face_index = face_index_map[map_index];
                    if (face_index != target_face_index) {
                        yi_s_other_edge = yi_s - 1;
                        pixel_value_other_outside = pixel_map[map_index];
                        break;
                    }
                }
                for (int yi_s = yi_s_edge; yi_s <= yi_s_other_edge; yi_s++) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int pixel_value_inside = pixel_map[map_index_s];
                    float delta_pj = grad_silhouette[map_index_s];
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
            int map_index = target_batch_index * image_width * image_height + yi_s_start * image_width + xi_p;
            int face_index = face_index_map[map_index];
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int yi_s = yi_s_start; yi_s >= yi_s_end; yi_s--) {
                    int map_index = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int face_index = face_index_map[map_index];
                    if (face_index == target_face_index) {
                        yi_s_edge = yi_s;
                        pixel_value_inside = pixel_map[map_index];
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int yi_s = yi_s_start; yi_s > yi_s_edge; yi_s--) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int pixel_value_outside = pixel_map[map_index_s];
                    float delta_ij = pixel_value_inside - pixel_value_outside;
                    float delta_pj = grad_silhouette[map_index_s];
                    if (delta_pj == 0) {
                        continue;
                    }
                    // 頂点Aについて
                    {
                        if (xi_p - xi_p_start > 0) {
                            float moving_distance = (yi_s - yi_s_edge) / (float)(xi_p - xi_p_start) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                    }
                    // 頂点Bについて
                    {
                        if (xi_p_end - xi_p > 0) {
                            float moving_distance = (yi_s - yi_s_edge) / (float)(xi_p_end - xi_p) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int map_index_outside = target_batch_index * image_width * image_height + (yi_s_edge + 1) * image_width + xi_p;
                int pixel_value_outside = pixel_map[map_index_outside];
                int yi_s_other_edge = 0;
                int pixel_value_other_outside = 0;
                // 反対側の辺の位置を特定する
                for (int yi_s = yi_s_edge - 1; yi_s >= yi_s_end; yi_s--) {
                    int map_index = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int face_index = face_index_map[map_index];
                    if (face_index != target_face_index) {
                        yi_s_other_edge = yi_s + 1;
                        pixel_value_other_outside = pixel_map[map_index];
                        break;
                    }
                }
                for (int yi_s = yi_s_edge; yi_s >= yi_s_other_edge; yi_s--) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_s * image_width + xi_p;
                    int pixel_value_inside = pixel_map[map_index_s];
                    float delta_pj = grad_silhouette[map_index_s];
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                        // 頂点Bについて
                        if (xi_p > xi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (yi_s - yi_s_other_edge) / (float)(xi_p - xi_c) * (float)(xi_p_end - xi_p_start);
                            if (moving_distance > 0) {
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3 + 1] += grad;
                                debug_grad_map[map_index_s] += grad;
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
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map)
{
    // 画像座標系に変換
    // 左上が原点で右下が(image_width, image_height)になる
    // 画像配列に合わせるためそのような座標系になる
    int xi_a = to_image_coordinate(xf_a, image_width);
    int xi_b = to_image_coordinate(xf_b, image_width);
    int xi_c = to_image_coordinate(xf_c, image_width);
    int yi_a = (image_height - 1) - to_image_coordinate(yf_a, image_height);
    int yi_b = (image_height - 1) - to_image_coordinate(yf_b, image_height);
    int yi_c = (image_height - 1) - to_image_coordinate(yf_c, image_height);

    if (yi_a == yi_b) {
        return;
    }

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
            int map_index = target_batch_index * image_width * image_height + yi_p * image_width + si_x_start;
            int face_index = face_index_map[map_index];
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int xi_s = si_x_start; xi_s <= si_x_end; xi_s++) {
                    int map_index = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int face_index = face_index_map[map_index];
                    if (face_index == target_face_index) {
                        xi_s_edge = xi_s;
                        pixel_value_inside = pixel_map[map_index];
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                for (int xi_s = si_x_start; xi_s < xi_s_edge; xi_s++) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int pixel_value_outside = pixel_map[map_index_s];
                    // 走査点と面の輝度値の差
                    float delta_ij = pixel_value_inside - pixel_value_outside;
                    // 頂点の実際の移動量を求める
                    // スキャンライン上の移動距離ではない
                    // 相似な三角形なのでy方向の比率から求まる
                    // 頂点Aについて
                    if (yi_p_end - yi_p > 0) {
                        float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                        if (moving_distance > 0) {
                            float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                            // 左側は勾配が逆向きになる
                            float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad;
                            debug_grad_map[map_index_s] += grad;
                        }
                    }
                    // 頂点Bについて
                    if (yi_p - yi_p_start > 0) {
                        float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                        if (moving_distance > 0) {
                            float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                            // 左側は勾配が逆向きになる
                            float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                            grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad;
                            debug_grad_map[map_index_s] += grad;
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int map_index_outside = target_batch_index * image_width * image_height + yi_p * image_width + xi_s_edge - 1;
                int pixel_value_outside = pixel_map[map_index_outside];
                int xi_s_other_edge = image_width - 1;
                int pixel_value_other_outside = 0;
                // 反対側の辺の位置を特定する
                for (int xi_s = xi_s_edge + 1; xi_s <= si_x_end; xi_s++) {
                    int map_index = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int face_index = face_index_map[map_index];
                    if (face_index != target_face_index) {
                        xi_s_other_edge = xi_s - 1;
                        pixel_value_other_outside = pixel_map[map_index];
                        break;
                    }
                }
                for (int xi_s = xi_s_edge; xi_s <= xi_s_other_edge; xi_s++) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int pixel_value_inside = pixel_map[map_index_s];
                    // 面の左側（頂点を右に動かしていって辺に当たる場合）
                    // 論文のdelta_ij_bに対応
                    {
                        float delta_ij = pixel_value_outside - pixel_value_inside;
                        // 頂点Aについて
                        // 頂点の実際の移動量を求める
                        // スキャンライン上の移動距離ではない
                        // 相似な三角形なのでy方向の比率から求まる
                        if (yi_p - yi_p_start > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                        // 頂点Bについて
                        // 頂点の実際の移動量を求める
                        // スキャンライン上の移動距離ではない
                        // 相似な三角形なのでy方向の比率から求まる
                        if (yi_p_end - yi_p > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p > yi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s_other_edge - xi_s) / (float)(yi_p - yi_c) * (float)(yi_p_end - yi_c);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
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
            int map_index = target_batch_index * image_width * image_height + yi_p * image_width + si_x_start;
            int face_index = face_index_map[map_index];
            if (face_index == target_face_index) {
                continue;
            }
            // 外側の全ての画素から勾配を求める
            {
                bool edge_found = false;
                int pixel_value_inside = 0;
                for (int xi_s = si_x_start; xi_s >= si_x_end; xi_s--) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int face_index = face_index_map[map_index_s];
                    if (face_index == target_face_index) {
                        xi_s_edge = xi_s;
                        pixel_value_inside = pixel_map[map_index_s];
                        edge_found = true;
                        break;
                    }
                }
                if (edge_found == false) {
                    continue;
                }
                if (xi_s_edge < si_x_start) {
                    for (int xi_s = si_x_start; xi_s > xi_s_edge; xi_s--) {
                        int map_index_s = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                        int pixel_value_outside = pixel_map[map_index_s];
                        float delta_ij = pixel_value_inside - pixel_value_outside;

                        // 頂点Aについて
                        if (yi_p - yi_p_start > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p - yi_p_start) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p_end - yi_p > 0) {
                            float moving_distance = (xi_s - xi_s_edge) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                    }
                }
            }
            // 内側の全ての画素から勾配を求める
            {
                int map_index_outside = target_batch_index * image_width * image_height + yi_p * image_width + xi_s_edge + 1;
                int pixel_value_outside = pixel_map[map_index_outside];
                int xi_s_other_edge = 0;
                int pixel_value_other_outside = 0;
                // 反対側の辺の位置を特定する
                for (int xi_s = xi_s_edge - 1; xi_s >= si_x_end; xi_s--) {
                    int map_index = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int face_index = face_index_map[map_index];
                    if (face_index != target_face_index) {
                        xi_s_other_edge = xi_s + 1;
                        pixel_value_other_outside = pixel_map[map_index];
                        break;
                    }
                }
                for (int xi_s = xi_s_edge; xi_s >= xi_s_other_edge; xi_s--) {
                    int map_index_s = target_batch_index * image_width * image_height + yi_p * image_width + xi_s;
                    int pixel_value_inside = pixel_map[map_index_s];
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
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p < yi_c) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s - xi_s_other_edge) / (float)(yi_p - yi_p_start) * (float)(yi_c - yi_p_start);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
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
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_a * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
                            }
                        }
                        // 頂点Bについて
                        if (yi_p_end - yi_p) {
                            // 頂点の実際の移動量を求める
                            // スキャンライン上の移動距離ではない
                            // 相似な三角形なのでy方向の比率から求まる
                            float moving_distance = (xi_s_edge - xi_s) / (float)(yi_p_end - yi_p) * (float)(yi_p_end - yi_p_start);
                            if (moving_distance > 0) {
                                float delta_pj = grad_silhouette[target_batch_index * image_width * image_height + yi_p * image_width + xi_s];
                                float grad = (delta_pj * delta_ij >= 0) ? 0 : -delta_pj * delta_ij / moving_distance / 255.0f;
                                grad_vertices[target_batch_index * num_vertices * 3 + vertex_index_b * 3] += grad;
                                debug_grad_map[map_index_s] += grad;
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
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map)
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
        face_index_map,
        pixel_map,
        grad_vertices,
        grad_silhouette,
        debug_grad_map);
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
        face_index_map,
        pixel_map,
        grad_vertices,
        grad_silhouette,
        debug_grad_map);
}

// シルエットの誤差から各頂点の勾配を求める
void cpp_backward_silhouette(
    int* faces,
    float* face_vertices,
    float* vertices,
    int* face_index_map,
    int* pixel_map,
    float* grad_vertices,
    float* grad_silhouette,
    float* debug_grad_map,
    int batch_size,
    int num_faces,
    int num_vertices,
    int image_width,
    int image_height)
{
    for (int batch_index = 0; batch_index < batch_size; batch_index++) {
        for (int face_index = 0; face_index < num_faces; face_index++) {
            int fv_index = batch_index * num_faces * 9 + face_index * 9;
            float xf_1 = face_vertices[fv_index + 0];
            float yf_1 = face_vertices[fv_index + 1];
            // float zf_1 = face_vertices[fv_index + 2];
            float xf_2 = face_vertices[fv_index + 3];
            float yf_2 = face_vertices[fv_index + 4];
            // float zf_2 = face_vertices[fv_index + 5];
            float xf_3 = face_vertices[fv_index + 6];
            float yf_3 = face_vertices[fv_index + 7];
            // float zf_3 = face_vertices[fv_index + 8];

            int v_index = batch_index * num_faces * 3 + face_index * 3;
            int vertex_1_index = faces[v_index + 0];
            int vertex_2_index = faces[v_index + 1];
            int vertex_3_index = faces[v_index + 2];

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
                face_index_map,
                pixel_map,
                grad_vertices,
                grad_silhouette,
                debug_grad_map);
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
                face_index_map,
                pixel_map,
                grad_vertices,
                grad_silhouette,
                debug_grad_map);
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
                face_index_map,
                pixel_map,
                grad_vertices,
                grad_silhouette,
                debug_grad_map);
        }
    }
}