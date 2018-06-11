#pragma once
#include "base/view.h"
#include "view/image.h"
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
    std::vector<std::tuple<View*, double, double, double, double>> _views;
    void _run();

public:
    Window();
    ~Window();
    void show();
    void add_view(view::ImageView* view, double x, double y, double width, double height);
};
}