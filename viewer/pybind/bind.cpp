#include "viewer.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;

PYBIND11_MODULE(viewer, m)
{
    py::class_<viewer::pybind::Viewer>(m, "Viewer")
        .def(py::init<>());
}