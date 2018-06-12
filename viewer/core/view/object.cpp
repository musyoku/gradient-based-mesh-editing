#include "object.h"
#include "../opengl.h"
#include <glm/glm.hpp>
#include <iostream>

namespace viewer {
namespace view {
    ObjectView::ObjectView(data::ObjectData* data, double x, double y, double width, double height)
        : View(x, y, width, height)
    {
        _data = data;
        _renderer = std::make_unique<renderer::ObjectRenderer>(data->vertices(), data->num_vertices(), data->faces(), data->num_faces());
    }
    void ObjectView::_bind_vertices()
    {
        _renderer->update_vertices(_data->vertices(), _data->num_vertices());
    }
    void ObjectView::_bind_faces()
    {
        _renderer->update_faces(_data->faces(), _data->num_faces());
    }
    void ObjectView::render(double aspect_ratio)
    {
        if (_data->vertices_updated()) {
            _bind_vertices();
        }
        if (_data->faces_updated()) {
            _bind_faces();
        }
        _renderer->render(aspect_ratio);
    }
}
}
