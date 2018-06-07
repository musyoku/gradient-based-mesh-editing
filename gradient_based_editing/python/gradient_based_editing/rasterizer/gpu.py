import string
import chainer
import cupy as xp


def forward_face_index_map_gpu(faces, face_index_map, depth_map,
                               silhouette_image):
    batch_size = faces.shape[0]
    image_width, image_height = depth_map.shape[1:3]
    loop = self.xp.arange(batch_size * image_width * image_height).astype(
        xp.in32)

        float* face_vertices,
    int* face_index_map,
    float* depth_map,
    int* silhouette_image,
    int batch_size,
    int num_faces,
    int image_width,
    int image_height

    xp.ElementwiseKernel(
        "int32 _, raw float32 face_vertices, raw int32 face_index_map, raw float32 depth_map, raw int32 silhouette_image, raw float32",
        "",
        preamble=string.Template("""
            __forceinline__ __device__
            float to_projected_coordinate(const int p, const int size)
            {
                return 2.0 * (p / (float)(size - 1) - 0.5);
            }

            // [-1, 1] -> [0, size - 1]
            __forceinline__ __device__
            int to_image_coordinate(const float p, const int size)
            {
                return std::min(std::max((int)std::round((p + 1.0f) * 0.5f * (size - 1)), 0), size - 1);
            }
        """),
        operation=string.Template("""
                    const int image_width = ${image_width};
                    const int image_height = ${image_height};
                    const int num_faces = ${num_faces};
                    const int batch_index = i / (image_width * image_height);
                    const int pixel_index = i % (image_width * image_height);
                    const int yi = pn / is;
                    const int xi = pn % is;
                    const float yp = (2. * yi + 1 - is) / is;
                    const float xp = (2. * xi + 1 - is) / is;
                    float* face = &face_vertices[batch_index * num_faces * 9];
                    for (int face_index = 0; face_index < num_faces; face_index++) {
                        const float xf_1 = face[0];
                        const float yf_1 = face[1];
                        const float zf_1 = face[2];
                        const float xf_2 = face[3];
                        const float yf_2 = face[4];
                        const float zf_2 = face[5];
                        const float xf_3 = face[6];
                        const float yf_3 = face[7];
                        const float zf_3 = face[8];

                        // カリングによる裏面のスキップ
                        // 面の頂点の並び（1 -> 2 -> 3）が時計回りの場合描画しない
                        if ((yf_1 - yf_3) * (xf_1 - xf_2) < (yf_1 - yf_2) * (xf_1 - xf_3)) {
                            continue;
                        }


                        // yi \in [0, image_height] -> yf \in [-1, 1]
                        const float yf = -to_projected_coordinate(yi, image_height);
                        // y座標が面の外部ならスキップ
                        if ((yf > yf_1 && yf > yf_2 && yf > yf_3) || (yf < yf_1 && yf < yf_2 && yf < yf_3)) {
                            continue;
                        }

                        // xi \in [0, image_width] -> xf \in [-1, 1]
                        float xf = to_projected_coordinate(xi, image_width);

                        // xyが面の外部ならスキップ
                        // Edge Functionで3辺のいずれかの右側にあればスキップ
                        // https://www.cs.drexel.edu/~david/Classes/Papers/comp175-06-pineda.pdf
                        if ((yf - yf_1) * (xf_2 - xf_1) < (xf - xf_1) * (yf_2 - yf_1) || (yf - yf_2) * (xf_3 - xf_2) < (xf - xf_2) * (yf_3 - yf_2) || (yf - yf_3) * (xf_1 - xf_3) < (xf - xf_3) * (yf_1 - yf_3)) {
                            continue;
                        }


                        /* go to next face */
                        face += 9;
                        face_inv += 9;
                        /* return if backside */
                        if ((face[7] - face[1]) * (face[3] - face[0]) < (face[4] - face[1]) * (face[6] - face[0]))
                            continue;
                        /* check [py, px] is inside the face */
                        if (((yp - face[1]) * (face[3] - face[0]) < (xp - face[0]) * (face[4] - face[1])) ||
                            ((yp - face[4]) * (face[6] - face[3]) < (xp - face[3]) * (face[7] - face[4])) ||
                            ((yp - face[7]) * (face[0] - face[6]) < (xp - face[6]) * (face[1] - face[7]))) continue;
                        /* compute w = face_inv * p */
                        float w[3];
                        for (int k = 0; k < 3; k++)
                        w[0] = face_inv[3 * 0 + 0] * xi + face_inv[3 * 0 + 1] * yi + face_inv[3 * 0 + 2];
                        w[1] = face_inv[3 * 1 + 0] * xi + face_inv[3 * 1 + 1] * yi + face_inv[3 * 1 + 2];
                        w[2] = face_inv[3 * 2 + 0] * xi + face_inv[3 * 2 + 1] * yi + face_inv[3 * 2 + 2];
                        /* sum(w) -> 1, 0 < w < 1 */
                        float w_sum = 0;
                        for (int k = 0; k < 3; k++) {
                            w[k] = min(max(w[k], 0.), 1.);
                            w_sum += w[k];
                        }
                        for (int k = 0; k < 3; k++) w[k] /= w_sum;
                        /* compute 1 / zp = sum(w / z) */
                        const float zp = 1. / (w[0] / face[2] + w[1] / face[5] + w[2] / face[8]);
                        if (zp <= ${near} || ${far} <= zp) continue;
                        /* check z-buffer */
                        if (zp < depth_min) {
                            depth_min = zp;
                            face_index_min = face_index;
                            for (int k = 0; k < 3; k++) weight_min[k] = w[k];
                            if (${return_depth}) for (int k = 0; k < 9; k++) face_inv_min[k] = face_inv[k];
                        }
                    }
                    /* set to global memory */
                    if (0 <= face_index_min) {
                        depth_map[i] = depth_min;
                        face_index_map[i] = face_index_min;
                        for (int k = 0; k < 3; k++) weight_map[3 * i + k] = weight_min[k];
                        if (${return_depth}) for (int k = 0; k < 9; k++) face_inv_map[9 * i + k] = face_inv_min[k];
                    }
                """).substitute(
            num_faces=self.num_faces,
            image_size=self.image_size,
            near=self.near,
            far=self.far,
            return_rgb=int(self.return_rgb),
            return_alpha=int(self.return_alpha),
            return_depth=int(self.return_depth),
        ),
        'function',
    )(loop, self.faces, faces_inv, self.face_index_map, self.weight_map,
      self.depth_map, self.face_inv_map)
