#pragma once
#include <gl3w/gl3w.h>
#include <memory>
#include <pybind11/numpy.h>

namespace viewer {
namespace data {
    class ObjectData {
    private:
        int _num_vertices;
        int _num_faces;
        bool _vertices_updated;
        bool _faces_updated;
        std::unique_ptr<GLfloat[]> _vertices;
        std::unique_ptr<GLuint[]> _faces;

    public:
        ObjectData(pybind11::array_t<GLfloat> vertices, int num_vertices, pybind11::array_t<GLuint> faces, int num_faces);
        void update_faces(pybind11::array_t<GLuint> faces);
        void update_vertices(pybind11::array_t<GLfloat> vertices);
        bool vertices_updated();
        bool faces_updated();
        int num_vertices();
        int num_faces();
        GLfloat* vertices();
        GLuint* faces();
    };
}
}