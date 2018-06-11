#include "../core/view/image.h"
#include "../core/window.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;

PYBIND11_MODULE(viewer, m)
{
    py::class_<viewer::view::Image>(m, "ImageView")
        .def(py::init<int, int, int>(), py::arg("height"), py::arg("width"), py::arg("num_channels"));

    py::class_<viewer::Window>(m, "Window")
        .def(py::init<>())
        .def("show", &viewer::Window::show);
}