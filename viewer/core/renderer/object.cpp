#include "object.h"
#include "../opengl.h"
#include <glm/gtc/matrix_transform.hpp>

namespace viewer {
namespace renderer {
    ObjectRenderer::ObjectRenderer(GLfloat* vertices, int num_vertices, GLuint* faces, int num_faces)
    {
        _num_faces = num_faces;
        _camera_location = glm::vec3(0.0, 0.0, 3.0);
        _view_mat = glm::lookAt(
            _camera_location,
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.0, 1.0, 0.0));

        const GLchar vertex_shader[] = R"(
#version 410
in vec3 position;
in vec3 normal_vector;
uniform mat4 pvm;
out float power;
void main(void)
{
    gl_Position = pvm * vec4(position, 1.0);
    vec3 light_direction = vec3(0.0, -1.0, -1.0);
    // vec3 normal = normalize((vec4(normal_vector, 0.0) * pvm).xyz);
    power = dot(normal_vector, -normalize(light_direction));
    power = clamp(power, 0.1, 1.0);
    // power = clamp((normal_vector + 1.0) / 2.0, 0.0, 1.0);
}
)";

        const GLchar fragment_shader[] = R"(
#version 410
in float power;
out vec4 frag_color;
void main(){
    frag_color = vec4(power * vec3(1.0), 1.0);
}
)";

        _program = opengl::create_program(vertex_shader, fragment_shader);

        _attribute_position = glGetAttribLocation(_program, "position");
        _attribute_normal_vector = glGetAttribLocation(_program, "normal_vector");
        _uniform_pvm = glGetUniformLocation(_program, "pvm");

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);

        glGenBuffers(1, &_vbo_faces);

        glGenBuffers(1, &_vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
        glVertexAttribPointer(_attribute_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_attribute_position);

        glGenBuffers(1, &_vbo_normal_vectors);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_normal_vectors);
        glVertexAttribPointer(_attribute_normal_vector, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(_attribute_normal_vector);

        glBindVertexArray(0);

        update_faces(faces, num_faces);
        update_vertices(vertices, num_vertices);
    }

    ObjectRenderer::~ObjectRenderer()
    {
        glDeleteBuffers(1, &_vbo_faces);
        glDeleteBuffers(1, &_vbo_normal_vectors);
        glDeleteBuffers(1, &_vbo_uv);
        glDeleteBuffers(1, &_vbo_vertices);
        glDeleteVertexArrays(1, &_vao);
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
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, 3 * num_vertices * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    void ObjectRenderer::update_normal_vectors(GLfloat* normal_vectors, int num_vertices)
    {
        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo_normal_vectors);
        glBufferData(GL_ARRAY_BUFFER, 3 * num_vertices * sizeof(GLfloat), normal_vectors, GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
    void ObjectRenderer::render(GLfloat aspect_ratio)
    {
        glUseProgram(_program);
        glBindVertexArray(_vao);

        glm::mat4 projection_mat = glm::perspective(
            90.0f,
            aspect_ratio,
            0.1f,
            100.0f);

        glm::mat4 pvm = projection_mat * _view_mat;
        glUniformMatrix4fv(_uniform_pvm, 1, GL_FALSE, &pvm[0][0]);

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
