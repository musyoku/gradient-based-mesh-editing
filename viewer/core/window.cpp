#include "window.h"
#include <external/glm/glm.hpp>

namespace viewer {
void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}
Window::Window(Figure* figure)
{
    _figure = figure;
    _closed = false;

    glfwSetErrorCallback(glfw_error_callback);
    if (!!glfwInit() == false) {
        std::runtime_error("Failed to initialize GLFW.");
        return;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    _window = glfwCreateWindow(1, 1, "", NULL, NULL);
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);
    gl3wInit();
}
Window::~Window()
{
    glfwSetWindowShouldClose(_shared_window, GL_TRUE);
    glfwSetWindowShouldClose(_window, GL_TRUE);
    _thread.join();
    glfwDestroyWindow(_window);
    glfwTerminate();
}
void Window::_run()
{
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    _shared_window = glfwCreateWindow(1920, 800, "Gradient-based Mesh Editing", NULL, _window);
    glfwMakeContextCurrent(_shared_window);

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& frame : _figure->_images) {
        data::ImageData* data = std::get<0>(frame);
        double x = std::get<1>(frame);
        double y = std::get<2>(frame);
        double width = std::get<3>(frame);
        double height = std::get<4>(frame);
        _image_views.emplace_back(std::make_unique<view::ImageView>(data, x, y, width, height));
    }

    while (!!glfwWindowShouldClose(_shared_window) == false) {
        glfwPollEvents();
        int screen_width, screen_height;
        glfwGetFramebufferSize(_shared_window, &screen_width, &screen_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, screen_width, screen_height);
        glClearColor(0.0, 0.0, 0.0, 0.0);

        // int window_width, window_height;
        // glfwGetWindowSize(_shared_window, &window_width, &window_height);

        for (const auto& view : _image_views) {
            int x = screen_width * view->x();
            int y = screen_height * view->y();
            int width = screen_width * view->width();
            int height = screen_height * view->height();
            glViewport(x, screen_height - y - height, width, height);
            double aspect_ratio = (double)height / (double)width;
            view->render(aspect_ratio);
        }

        glfwSwapBuffers(_shared_window);
    }
    glfwDestroyWindow(_shared_window);
    _closed = true;
}
void Window::show()
{
    try {
        _thread = std::thread(&Window::_run, this);
    } catch (const std::system_error& e) {
        std::runtime_error(e.what());
    }
}
bool Window::closed()
{
    return _closed;
}
}