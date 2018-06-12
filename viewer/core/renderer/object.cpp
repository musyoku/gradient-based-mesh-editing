#include "object.h"
#include "../opengl.h"
#include <glm/gtc/matrix_transform.hpp>

namespace viewer {
namespace renderer {
    ObjectRenderer::ObjectRenderer(GLfloat* vertices, int num_vertices, GLuint* faces, int num_faces)
    {
        _num_faces = num_faces;
        _camera_location = glm::vec3(1.0, 1.0, 1.0);
        _view_mat = glm::lookAt(
            _camera_location,
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.0, 1.0, 0.0));

        const GLchar vertex_shader[] = R"(
#version 410
in vec3 position;
uniform mat4 mat;
void main(void)
{
    gl_Position = mat * vec4(position, 1.0);
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

        glGenBuffers(1, &_vbo_faces);
        glGenBuffers(1, &_vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
        glVertexAttribPointer(_attribute_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_attribute_position);

        glBindVertexArray(0);

        update_faces(faces, num_faces);
        update_vertices(vertices, num_vertices);
    }
    void ObjectRenderer::update_faces(GLuint* faces, int num_faces)
    {
        glBindVertexArray(_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_faces);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * num_faces * sizeof(GLuint), faces, GL_STATIC_DRAW);
        glBindVertexArray(0);
        _num_faces = num_faces;
    }
    void ObjectRenderer::update_vertices(GLfloat* vertices, int num_vertices)
    {
        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, 3 * num_vertices * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
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

        glUniformMatrix4fv(_uniform_mat, 1, GL_FALSE, &_view_mat[0][0]);

        glDrawElements(GL_TRIANGLES, 3 * _num_faces, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glUseProgram(0);
    }
    void ObjectRenderer::zoom_in()
    {
        _camera_location = _camera_location + glm::vec3(0.0, 0.0, 0.1);
        _view_mat = glm::lookAt(
            _camera_location,
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.0, 1.0, 0.0));
    }
    void ObjectRenderer::zoom_out()
    {
        _camera_location = _camera_location - glm::vec3(0.0, 0.0, 0.1);
        _view_mat = glm::lookAt(
            _camera_location,
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.0, 1.0, 0.0));
    }
}
}
