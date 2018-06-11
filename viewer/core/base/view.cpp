#include "view.h"
#include <stdexcept>

namespace viewer {
View::View(int x, int y, int width, int height)
{
    _x = x;
    _y = y;
    _width = width;
    _height = height;
}
int View::x()
{
    return _x;
}
int View::y()
{
    return _y;
}
int View::width()
{
    return _width;
}
int View::height()
{
    return _height;
}
void View::render()
{
    std::runtime_error("Function `render` must be overridden.");
}
}