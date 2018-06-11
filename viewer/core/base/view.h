#pragma once
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>

namespace viewer {
class View {
public:
    virtual void render(GLFWwindow* window);
};
}