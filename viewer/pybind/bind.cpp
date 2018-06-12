#include "../core/figure.h"
#include "../core/view/image.h"
#include "../core/window.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;
using namespace viewer;

PYBIND11_MODULE(viewer, m)
{
    py::class_<data::ImageData>(m, "ImageData")
        .def(py::init<int, int, int>(), py::arg("height"), py::arg("width"), py::arg("num_channels"))
        .def("set", &data::ImageData::resize)
        .def("set", &data::ImageData::update)
        .def("set", &data::ImageData::set);

    py::class_<Figure>(m, "Figure")
        .def(py::init<>())
        .def("add", &Figure::add);

    py::class_<Window>(m, "Window")
        .def(py::init<Figure*>())
        .def("closed", &Window::closed)
        .def("show", &Window::show);
}