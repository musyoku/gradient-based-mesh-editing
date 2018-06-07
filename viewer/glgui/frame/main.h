#pragma once
#include "../component/parameters.h"
#include "../component/noise.h"
#include "../component/method.h"

namespace glgui {
namespace frame {
    class Main {
    public:
        bool _is_slam_running;
        component::Parameters* _parameters;
        component::Noise* _noise;
        component::Method* _method;
        Main(component::Parameters* parameters, component::Method* method, component::Noise* noise);
        void render();
    };
}
}