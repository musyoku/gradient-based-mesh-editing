#pragma once
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>
#include <iostream>
#include <thread>

namespace viewer {
namespace pybind {
    void glfw_error_callback(int error, const char* description);
    class Viewer {
    protected:
        std::thread _thread;
        GLFWwindow* _window;

    public:
        Viewer();
        ~Viewer();
        void run();
    };
}
}