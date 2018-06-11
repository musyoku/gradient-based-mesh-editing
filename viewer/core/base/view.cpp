#include "view.h"
#include <stdexcept>

namespace viewer {
void View::render(GLFWwindow* window)
{
    std::runtime_error("Function `render` must be overridden.");
}
}