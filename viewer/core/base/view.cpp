#include "view.h"
#include <stdexcept>

namespace viewer {
View::View(double x, double y, double width, double height)
{
    _x = x;
    _y = y;
    _width = width;
    _height = height;
}
double View::x()
{
    return _x;
}
double View::y()
{
    return _y;
}
double View::width()
{
    return _width;
}
double View::height()
{
    return _height;
}
void View::render(double aspect_ratio)
{
    std::runtime_error("Function `render` must be overridden.");
}
}