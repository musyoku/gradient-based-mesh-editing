#pragma once
#include "figure.h"
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>
#include <iostream>
#include <thread>
#include <vector>

namespace viewer {
void glfw_error_callback(int error, const char* description);
class Window {
protected:
    std::thread _thread;
    GLFWwindow* _window;
    GLFWwindow* _shared_window;
    Figure* _figure;
    std::vector<std::unique_ptr<view::ImageView>> _image_views;
    bool _closed;
    void _run();

public:
    Window(Figure* figure);
    ~Window();
    void show();
    bool closed();
};
}