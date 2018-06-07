#include "../core/opengl.h"
#include "image.h"

namespace glgui {
namespace view {
    Image::Image()
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
uniform sampler2D texture;
in vec2 coord;
out vec4 color;
void main(){
    color = texture2D(texture, coord);
}
)";

        _program = opengl::create_program(vertex_shader, fragment_shader);

        const GLfloat uv[] = {
            1,
            1,
            0,
            1,
            1,
            0,
            0,
            0,
        };

        _position_location = glGetAttribLocation(_program, "position");
        _uv_location = glGetAttribLocation(_program, "uv");
        _texture_location = glGetUniformLocation(_program, "texture");

        glUniform1i(_texture_location, 0); // = use GL_TEXTURE0

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        glGenBuffers(1, &_vbo_position);

        glGenBuffers(1, &_vbo_uv);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_uv);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), uv, GL_STATIC_DRAW);
        glVertexAttribPointer(_uv_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_uv_location);

        /* テクスチャの読み込みに使う配列 */
        GLubyte pixels[512 * 512 * 4];
        FILE* fp;

        /* テクスチャ画像の読み込み */
        if ((fp = fopen("robot.raw", "rb")) != NULL) {
            fread(pixels, 512 * 512 * 4, 1, fp);
            fclose(fp);
        } else {
            perror("robot.raw");
        }

        glGenTextures(1, &_texture_id);
        glBindTexture(GL_TEXTURE_2D, _texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA, 512, 512, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

        // setup sampler
        glGenSamplers(1, &_sampler_id);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(_sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
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
        glBindSampler(0, _sampler_id);

        // vertices
        glBindVertexArray(_vao);

        GLfloat position[][2] = {
            { -1.0, -1.0 },
            { 1.0, -1.0 },
            { -1.0, 1.0 },
            { 1.0, 1.0 }
        };

        glBindBuffer(GL_ARRAY_BUFFER, _vbo_position);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), position, GL_STATIC_DRAW);
        glVertexAttribPointer(_position_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_position_location);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        glUseProgram(0);
    }
}
}
