#pragma once
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>
#include <iostream>
#include <thread>

namespace viewer {
void glfw_error_callback(int error, const char* description);
class Window {
protected:
    std::thread _thread;
    GLFWwindow* _window;
    GLFWwindow* _shared_window;
    void _run();

public:
    Window();
    ~Window();
    void show();
};
}