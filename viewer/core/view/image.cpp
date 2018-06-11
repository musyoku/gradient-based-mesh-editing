#include "image.h"
#include "../opengl.h"
#include <external/glm/glm.hpp>
#include <iostream>

namespace viewer {
namespace view {
    Image::Image(int height, int width, int num_channels)
    {
        if (num_channels != 1 && num_channels != 3) {
            std::runtime_error("num_channels != 1 && num_channels != 3");
        }
        if (height <= 0) {
            std::runtime_error("height <= 0");
        }
        if (width <= 0) {
            std::runtime_error("width <= 0");
        }
        _height = height;
        _width = width;
        _num_channels = num_channels;
        _data = std::make_unique<GLubyte[]>(height * width * 3);

        const GLchar vertex_shader[]
            = R"(
#version 400
in vec2 position;
in vec2 uv;
out vec2 coord;
void main(void)
{
    gl_Position = vec4(position, 1.0, 1.0);
    coord = uv;
}
)";

        const GLchar fragment_shader[] = R"(
#version 400
uniform sampler2D image;
in vec2 coord;
out vec4 color;
void main(){
    color = texture(image, coord);
}
)";

        _program = opengl::create_program(vertex_shader, fragment_shader);

        const GLfloat uv[][2] = {
            { 1, 1 },
            { 0, 1 },
            { 1, 0 },
            { 0, 0 },
        };

        const GLfloat position[][2] = {
            { -1.0, -1.0 },
            { 1.0, -1.0 },
            { -1.0, 1.0 },
            { 1.0, 1.0 }
        };

        _attribute_position = glGetAttribLocation(_program, "position");
        _attribute_uv = glGetAttribLocation(_program, "uv");
        _attribute_image = glGetUniformLocation(_program, "image");
        _texture_unit = 0;

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        glGenBuffers(1, &_vbo_position);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_position);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), position, GL_STATIC_DRAW);
        glVertexAttribPointer(_attribute_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_attribute_position);

        glGenBuffers(1, &_vbo_uv);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_uv);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), uv, GL_STATIC_DRAW);
        glVertexAttribPointer(_attribute_uv, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_attribute_uv);

        glGenTextures(1, &_texture_id);
        glBindTexture(GL_TEXTURE_2D, _texture_id);
        glUniform1i(_attribute_image, _texture_unit);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        glGenSamplers(1, &_sampler_id);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    void Image::resize(int height, int width, int num_channels)
    {
        _data = std::make_unique<GLubyte[]>(height * width * 3);
    }
    void Image::set_data(pybind11::array_t<GLubyte> data)
    {
        auto size = data.size();
        if (size != _height * _width * _num_channels) {
            std::runtime_error("`data.size` muse be equal to `_height * _width * _num_channels`.");
        }
        if (data.ndim() < 2 || data.ndim() > 3) {
            std::runtime_error("(data.ndim() < 2 || data.ndim() > 3) -> false");
        }
        if (data.ndim() == 2 && _num_channels != 1) {
            std::runtime_error("(data.ndim() == 2 && _num_channels != 1) -> false");
        }
        if (data.ndim() == 3 && _num_channels != 3) {
            std::runtime_error("(data.ndim() == 3 && _num_channels != 3) -> false");
        }
        if (data.ndim() == 2) {
            auto ptr = data.mutable_unchecked<2>();
            for (ssize_t h = 0; h < data.shape(0); h++) {
                for (ssize_t w = 0; w < data.shape(1); w++) {
                    ssize_t index = h * _width + w;
                    GLubyte intensity = ptr(h, w);
                    _data[index * 3 + 0] = intensity;
                    _data[index * 3 + 1] = intensity;
                    _data[index * 3 + 2] = intensity;
                }
            }
        } else {
            auto ptr = data.mutable_unchecked<3>();
            for (ssize_t h = 0; h < data.shape(0); h++) {
                for (ssize_t w = 0; w < data.shape(1); w++) {
                    for (ssize_t c = 0; c < data.shape(2); c++) {
                        ssize_t index = h * _width * _num_channels + w * _num_channels + c;
                        _data[index] = ptr(h, w, c);
                    }
                }
            }
        }

        glBindTexture(GL_TEXTURE_2D, _texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGB, _height, _width, 0,
            GL_RGB, GL_UNSIGNED_BYTE, _data.get());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void Image::update(pybind11::array_t<GLubyte> data, int height, int width, int num_channels)
    {
        resize(height, width, num_channels);
        set_data(data);
    }

    void Image::render(GLFWwindow* window, int x, int y, int width, int height)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        int window_width, window_height;
        glfwGetWindowSize(window, &window_width, &window_height);

        glUseProgram(_program);
        glViewport(x, window_height - y - height, width, height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _texture_id);
        glBindSampler(_texture_unit, _sampler_id);

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        glUseProgram(0);
    }
}
}
