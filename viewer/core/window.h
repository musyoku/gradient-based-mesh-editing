#pragma once
#include "base/view.h"
#include "data/image.h"
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
    std::vector<std::tuple<data::ImageData*, double, double, double, double>> _image_frames;
    std::vector<std::unique_ptr<view::ImageView>> _image_views;
    void _run();

public:
    Window();
    ~Window();
    void show();
    void add_view(data::ImageData* data, double x, double y, double width, double height);
};
}