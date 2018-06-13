#include "../core/rasterize.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;

PYBIND11_MODULE(rasterize_cpu, module)
{
    module.def("forward_face_index_map", &gme::forward_face_index_map);
    module.def("backward_silhouette", &gme::backward_silhouette);
}