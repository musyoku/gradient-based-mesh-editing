#include "window.h"
#include <external/glm/glm.hpp>
#include <iostream>

namespace viewer {
void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}
Window::Window()
{
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
    glm::vec4 bg_color = glm::vec4(61.0f / 255.0f, 57.0f / 255.0f, 90.0f / 255.0f, 1.00f);
    while (!!glfwWindowShouldClose(_shared_window) == false) {
        glfwPollEvents();
        int screen_width, screen_height;
        glfwGetFramebufferSize(_shared_window, &screen_width, &screen_height);
        glViewport(0, 0, screen_width, screen_height);
        glClearColor(bg_color.x, bg_color.y, bg_color.z, bg_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto& item : _views) {
            View* view = std::get<0>(item);
            double _x = std::get<1>(item);
            double _y = std::get<2>(item);
            double _width = std::get<3>(item);
            double _height = std::get<4>(item);
            int x = screen_width * _x;
            int y = screen_height * _y;
            int width = screen_width * _width;
            int height = screen_height * _height;
            int window_width, window_height;
            glfwGetWindowSize(_shared_window, &window_width, &window_height);
            glViewport(x, window_height - y - height, width, height);
            view->render(_shared_window);
        }

        glfwSwapBuffers(_shared_window);
    }
}
void Window::add_view(view::ImageView* view, double x, double y, double width, double height)
{
    _views.emplace_back(std::make_tuple(view, x, y, width, height));
}
void Window::show()
{
    try {
        _thread = std::thread(&Window::_run, this);
    } catch (const std::system_error& e) {
        std::runtime_error(e.what());
    }
}
}