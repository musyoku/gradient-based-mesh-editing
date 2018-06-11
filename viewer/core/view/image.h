#pragma once
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>
#include <memory>
#include <pybind11/numpy.h>

namespace viewer {
namespace view {
    class Image {
    private:
        int _height;
        int _width;
        int _num_channels;
        std::unique_ptr<GLubyte[]> _data;
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
        Image(int height, int width, int num_channels);
        void resize(int height, int width, int num_channels);
        void set_data(pybind11::array_t<GLubyte> data);
        void update(pybind11::array_t<GLubyte> data, int height, int width, int num_channels);
        void render(GLFWwindow* window, int x, int y, int width, int height);
    };
}
}