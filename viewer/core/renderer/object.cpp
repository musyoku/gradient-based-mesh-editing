#include "object.h"
#include "../opengl.h"

namespace viewer {
namespace renderer {
    ObjectRenderer::ObjectRenderer(GLfloat* vertices, int num_vertices, GLuint* faces, int num_faces)
    {
        const GLchar vertex_shader[] = R"(
#version 410
in vec2 position;
uniform mat4 mat;
void main(void)
{
    gl_Position = mat * vec4(position, 0.5, 1.0);
}
)";

        const GLchar fragment_shader[] = R"(
#version 410
out vec4 color;
void main(){
    color = vec4(0.0, 1.0, 1.0, 1.0);
}
)";

        _program = opengl::create_program(vertex_shader, fragment_shader);

        _attribute_position = glGetAttribLocation(_program, "position");
        _uniform_mat = glGetUniformLocation(_program, "mat");

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        glGenBuffers(1, &_vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
        glVertexAttribPointer(_attribute_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_attribute_position);
        update_vertices(vertices, num_vertices);

        glGenBuffers(1, &_vbo_faces);
        update_faces(faces, num_faces);

        glBindVertexArray(0);
    }
    void ObjectRenderer::update_faces(GLuint* faces, int num_faces)
    {
        glBindVertexArray(_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_faces);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * num_faces * sizeof(GLuint), faces, GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    void ObjectRenderer::update_vertices(GLfloat* vertices, int num_vertices)
    {
        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_faces);
        glBufferData(GL_ARRAY_BUFFER, 3 * num_vertices * sizeof(GLuint), vertices, GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    void ObjectRenderer::render(GLfloat aspect_ratio)
    {
        glUseProgram(_program);
        glBindVertexArray(_vao);

        const GLfloat mat[] = {
            aspect_ratio, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        glUniformMatrix4fv(_uniform_mat, 1, GL_TRUE, mat);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glUseProgram(0);
    }
}
}
