#include "object.h"

namespace viewer {
namespace data {
    ObjectData::ObjectData(pybind11::array_t<GLfloat> vertices, int num_vertices, pybind11::array_t<GLuint> faces, int num_faces)
    {
        _num_vertices = num_vertices;
        _num_faces = num_faces;
        _vertices = std::make_unique<GLfloat[]>(num_vertices * 3);
        _faces = std::make_unique<GLuint[]>(num_faces * 3);
        update_vertices(vertices);
        update_faces(faces);
    }
    void ObjectData::update_vertices(pybind11::array_t<GLfloat> vertices)
    {
        auto size = vertices.size();
        if (size != _num_vertices * 3) {
            std::runtime_error("`vertices.size` muse be equal to `_num_faces * 3`.");
        }
        if (vertices.ndim() != 2) {
            std::runtime_error("(vertices.ndim() != 2) -> false");
        }
        auto ptr = vertices.mutable_unchecked<2>();
        for (ssize_t n = 0; n < vertices.shape(0); n++) {
            for (ssize_t p = 0; p < vertices.shape(1); p++) {
                ssize_t index = n * 3 + p;
                _vertices[index] = ptr(n, p);
            }
        }
        _vertices_updated = true;
    }
    void ObjectData::update_faces(pybind11::array_t<GLuint> faces)
    {
        auto size = faces.size();
        if (size != _num_faces * 3) {
            std::runtime_error("`faces.size` muse be equal to `_num_faces * 3`.");
        }
        if (faces.ndim() != 2) {
            std::runtime_error("(faces.ndim() != 2) -> false");
        }
        auto ptr = faces.mutable_unchecked<2>();
        for (ssize_t n = 0; n < faces.shape(0); n++) {
            for (ssize_t p = 0; p < faces.shape(1); p++) {
                ssize_t index = n * 3 + p;
                _faces[index] = ptr(n, p);
            }
        }
        _faces_updated = true;
    }

    int ObjectData::num_vertices()
    {
        return _num_vertices;
    }
    int ObjectData::num_faces()
    {
        return _num_faces;
    }
    bool ObjectData::vertices_updated()
    {
        bool ret = _vertices_updated;
        _vertices_updated = false;
        return ret;
    }
    bool ObjectData::faces_updated()
    {
        bool ret = _faces_updated;
        _faces_updated = false;
        return ret;
    }
}
}
