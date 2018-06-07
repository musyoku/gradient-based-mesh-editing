#include "viewer.h"
#include <external/glm/glm.hpp>
#include <external/imgui/impl.h>
#include <iostream>

namespace viewer {
namespace pybind {
    void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error %d: %s\n", error, description);
    }
    Viewer::Viewer()
    {
        try {
            _thread = std::thread(&Viewer::run, this);
        } catch (const std::system_error& e) {
            std::runtime_error(e.what());
        }
    }
    Viewer::~Viewer()
    {
        glfwSetWindowShouldClose(_window, GL_TRUE);
        _thread.join();
    }
    void Viewer::run()
    {
        // polygon rendering
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0f);

        glfwSetErrorCallback(glfw_error_callback);
        if (!!glfwInit() == false) {
            std::runtime_error("Failed to initialize GLFW.");
            return;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        _window = glfwCreateWindow(1920, 800, "slam2d", NULL, NULL);
        glfwMakeContextCurrent(_window);
        glfwSwapInterval(1); // Enable vsync
        gl3wInit();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        // ImGuiIO& io = ImGui::GetIO();
        // (void)io;
        ImGui_ImplGlfwGL3_Init(_window, true);
        ImGui::StyleColorsDark();

        glm::vec4 bg_color = glm::vec4(61.0f / 255.0f, 57.0f / 255.0f, 90.0f / 255.0f, 1.00f);

        while (!!glfwWindowShouldClose(_window) == false) {
            glfwPollEvents();
            ImGui_ImplGlfwGL3_NewFrame();

            int screen_width, screen_height;
            glfwGetFramebufferSize(_window, &screen_width, &screen_height);
            glViewport(0, 0, screen_width, screen_height);
            glClearColor(bg_color.x, bg_color.y, bg_color.z, bg_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            glViewport(0, 0, screen_width, screen_height);
            ImGui::Render();
            ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(_window);
        }

        // Cleanup
        ImGui_ImplGlfwGL3_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(_window);
        glfwTerminate();
    }
}
}