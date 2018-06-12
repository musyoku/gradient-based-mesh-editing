#pragma once
#include <gl3w/gl3w.h>
#include <memory>
#include <pybind11/numpy.h>

namespace viewer {
namespace data {
    class MeshData {
    private:
        int _num_vertices;
        int _num_faces;
        bool _updated;
        std::unique_ptr<GLfloat[]> _vertices;
        std::unique_ptr<GLuint[]> _faces;

    public:
        MeshData(int num_vertices, int num_faces);
        void update_faces(pybind11::array_t<GLuint> vertices);
        void update_vertices(pybind11::array_t<GLfloat> faces);
        bool updated();
        GLfloat* vertices();
        GLuint* faces();
    };
}
}