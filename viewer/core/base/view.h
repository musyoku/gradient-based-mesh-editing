#pragma once

namespace viewer {
class View {
protected:
    double _x;
    double _y;
    double _width;
    double _height;

public:
    View(double x, double y, double width, double height);
    double x();
    double y();
    double width();
    double height();
    virtual void render(double aspect_ratio);
};
}