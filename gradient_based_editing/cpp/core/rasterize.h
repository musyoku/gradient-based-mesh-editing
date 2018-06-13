#pragma once
#include <pybind11/numpy.h>

namespace gme {
void forward_face_index_map(
    pybind11::array_t<float> np_faces_vertices,
    pybind11::array_t<int> np_face_index_map,
    pybind11::array_t<float> np_depth_map,
    pybind11::array_t<int> np_silhouette_image);

void backward_silhouette(
    pybind11::array_t<int> np_faces,
    pybind11::array_t<float> np_face_vertices,
    pybind11::array_t<float> np_vertices,
    pybind11::array_t<int> np_face_index_map,
    pybind11::array_t<int> np_pixel_map,
    pybind11::array_t<float> np_grad_vertices,
    pybind11::array_t<float> np_grad_silhouette,
    pybind11::array_t<float> np_debug_grad_map);
}