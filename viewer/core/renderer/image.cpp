#include "image.h"
#include "../opengl.h"
#include <external/glm/glm.hpp>
#include <iostream>

namespace viewer {
namespace renderer {
    ImageRenderer::ImageRenderer()
    {
        const GLchar vertex_shader[] = R"(
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
    color = vec4(1.0);
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
    void ImageRenderer::set_data(GLubyte* data, int height, int width)
    {
        glBindTexture(GL_TEXTURE_2D, _texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGB, height, width, 0,
            GL_RGB, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void ImageRenderer::render(GLFWwindow* window)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(_program);
        
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
