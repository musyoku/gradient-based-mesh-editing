#pragma once
#include <pybind11/numpy.h>

namespace gme {
namespace py = pybind11;
void forward_face_index_map(
    py::array_t<float, py::array::c_style> np_faces_vertices,
    py::array_t<int, py::array::c_style> np_face_index_map,
    py::array_t<float, py::array::c_style> np_depth_map,
    py::array_t<int, py::array::c_style> np_silhouette_image);

void backward_silhouette(
    py::array_t<int, py::array::c_style> np_faces,
    py::array_t<float, py::array::c_style> np_face_vertices,
    py::array_t<float, py::array::c_style> np_vertices,
    py::array_t<int, py::array::c_style> np_face_index_map,
    py::array_t<int, py::array::c_style> np_pixel_map,
    py::array_t<float, py::array::c_style> np_grad_vertices,
    py::array_t<float, py::array::c_style> np_grad_silhouette,
    py::array_t<float, py::array::c_style> np_debug_grad_map);
}