#pragma once
#include "base/view.h"
#include "data/image.h"
#include "view/image.h"
#include <iostream>
#include <vector>

namespace viewer {
class Figure {
public:
    std::vector<std::tuple<data::ImageData*, double, double, double, double>> _images;
    void add(data::ImageData* data, double x, double y, double width, double height);
};
}