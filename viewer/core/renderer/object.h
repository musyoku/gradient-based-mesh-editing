#pragma once
#include <gl3w/gl3w.h>
#include <glfw/glfw3.h>

namespace viewer {
namespace renderer {
    class ObjectRenderer {
    private:
        GLuint _program;
        GLuint _attribute_position;
        GLuint _uniform_mat;
        GLuint _vao;
        GLuint _vbo_vertices;
        GLuint _vbo_faces;
        GLuint _vbo_uv;

    public:
        ObjectRenderer(GLfloat* vertices, int num_vertices, GLuint* faces, int num_faces);
        void update_faces(GLuint* faces, int num_faces);
        void update_vertices(GLfloat* vertices, int num_vertices);
        void render(GLfloat aspect_ratio);
    };
}
}