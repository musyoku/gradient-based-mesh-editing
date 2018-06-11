#pragma once
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>

namespace viewer {
namespace renderer {
    class ImageRenderer {
    private:
        GLuint _program;
        GLuint _attribute_image;
        GLuint _attribute_uv;
        GLuint _attribute_position;
        GLuint _vao;
        GLuint _vbo_position;
        GLuint _vbo_uv;
        GLuint _vbo_indices;
        GLuint _texture_id;
        GLuint _texture_unit;
        GLuint _sampler_id;

    public:
        ImageRenderer();
        void set_data(GLubyte* data, int height, int width);
        void render(GLFWwindow* window);
    };
}
}