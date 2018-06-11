#pragma once
#include "../base/view.h"
#include "../renderer/image.h"
#include <external/gl3w/gl3w.h>
#include <external/glfw/glfw3.h>
#include <memory>
#include <pybind11/numpy.h>

namespace viewer {
namespace view {
    class ImageView : public View {
    private:
        int _height;
        int _width;
        int _num_channels;
        std::unique_ptr<GLubyte[]> _data;
        std::unique_ptr<renderer::ImageRenderer> _renderer;

    public:
        ImageView(int height, int width, int num_channels);
        void resize(int height, int width, int num_channels);
        void set_data(pybind11::array_t<GLubyte> data);
        void update(pybind11::array_t<GLubyte> data, int height, int width, int num_channels);
        void render(GLFWwindow* window);
    };
}
}