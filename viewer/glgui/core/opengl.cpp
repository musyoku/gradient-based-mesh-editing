#include "opengl.h"

namespace glgui {
namespace opengl {
    // シェーダオブジェクトのコンパイル結果を表示する
    GLboolean print_shader_info_log(GLuint shader, const char* str)
    {
        // コンパイル結果を取得する
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            std::cout << "Compile Error in " << str << std::endl;

        // シェーダのコンパイル時のログの長さを取得する
        GLsizei bufSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufSize);

        if (bufSize > 1) {
            // シェーダのコンパイル時のログの内容を取得する
            GLchar* infoLog = new GLchar[bufSize];
            GLsizei length;
            glGetShaderInfoLog(shader, bufSize, &length, infoLog);
            std::cout << infoLog << std::endl;
            delete[] infoLog;
        }

        return (GLboolean)status;
    }
    GLboolean print_program_info_log(GLuint program)
    {
        // リンク結果を取得する
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
            std::cout << "Link Error" << std::endl;

        // シェーダのリンク時のログの長さを取得する
        GLsizei bufSize;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufSize);

        if (bufSize > 1) {
            // シェーダのリンク時のログの内容を取得する
            GLchar* infoLog = new GLchar[bufSize];
            GLsizei length;
            glGetProgramInfoLog(program, bufSize, &length, infoLog);
            std::cout << infoLog << std::endl;
            delete[] infoLog;
        }

        return (GLboolean)status;
    }
    GLuint create_program(const char* vertex_shader, const char* fragment_shader)
    {
        // バーテックスシェーダのシェーダオブジェクト
        GLuint vobj = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vobj, 1, &vertex_shader, NULL);
        glCompileShader(vobj);
        print_shader_info_log(vobj, "vertex shader");

        // フラグメントシェーダのシェーダオブジェクトの作成
        GLuint fobj = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fobj, 1, &fragment_shader, NULL);
        glCompileShader(fobj);
        print_shader_info_log(fobj, "fragment shader");

        // シェーダオブジェクトの取り付け
        GLuint program = glCreateProgram();
        glAttachShader(program, vobj);
        glDeleteShader(vobj);
        glAttachShader(program, fobj);
        glDeleteShader(fobj);

        // プログラムオブジェクトのリンク
        glLinkProgram(program);
        print_program_info_log(program);
        glUseProgram(program);

        return program;
    }
}
}