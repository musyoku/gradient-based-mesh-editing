#pragma once
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>
#include <external/glm/glm.hpp>

namespace glgui {
namespace view {
    class Image {
    public:
        GLuint _program;
        GLuint _position_location;
        GLuint _uv_location;
        GLuint _texture_location;
        GLuint _vao;
        GLuint _vbo_position;
        GLuint _vbo_uv;
        GLuint _vbo_indices;
        GLuint _texture_id;
        GLuint _sampler_id;
        Image();
        void render(GLFWwindow* window, int x, int y, int width, int height);
    };
}
}