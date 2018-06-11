#pragma once

namespace viewer {
class View {
protected:
    int _x;
    int _y;
    int _width;
    int _height;

public:
    View(int x, int y, int width, int height);
    int x();
    int y();
    int width();
    int height();
    virtual void render();
};
}