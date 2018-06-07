#include "main.h"
#include <external/imgui/imgui.h>

namespace glgui {
namespace frame {
    Main::Main(component::Parameters* parameters, component::Method* method, component::Noise* noise)
    {
        _parameters = parameters;
        _noise = noise;
        _method = method;
        _is_slam_running = false;
    }
    void Main::render()
    {
        if (_is_slam_running) {
            if (ImGui::Button("Stop")) {
                _is_slam_running = false;
            }
        } else {
            if (ImGui::Button("Start")) {
                _is_slam_running = true;
            }
        }
        _parameters->render();
        _method->render();
        _noise->render();
    }
}
}