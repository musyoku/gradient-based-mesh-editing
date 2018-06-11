#include "image.h"
#include "../opengl.h"
#include <external/glm/glm.hpp>
#include <iostream>

namespace viewer {
namespace view {
    ImageView::ImageView(data::ImageData* data, double x, double y, double width, double height)
        : View(x, y, width, height)
    {
        _data = data;
        _renderer = std::make_unique<renderer::ImageRenderer>();
    }
    void ImageView::_bind_data()
    {
        _renderer->set_data(_data->raw(), _data->height(), _data->width());
    }
    void ImageView::render()
    {
        if (_data->updated()) {
            _bind_data();
        }
        _renderer->render();
    }
}
}
