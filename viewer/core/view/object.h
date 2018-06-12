#pragma once
#include "../base/view.h"
#include "../data/object.h"
#include "../renderer/object.h"
#include <gl3w/gl3w.h>
#include <glfw/glfw3.h>
#include <memory>
#include <pybind11/numpy.h>

namespace viewer {
namespace view {
    class ObjectView : public View {
    private:
        data::ObjectData* _data;
        std::unique_ptr<renderer::ObjectRenderer> _renderer;
        void _bind_vertices();
        void _bind_faces();

    public:
        ObjectView(data::ObjectData* data, double x, double y, double width, double height);
        void zoom_in();
        void zoom_out();
        virtual void render(double aspect_ratio);
    };
}
}