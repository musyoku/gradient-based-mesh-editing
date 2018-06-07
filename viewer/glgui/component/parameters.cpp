#include "parameters.h"
#include <external/imgui/imgui.h>

namespace glgui {
namespace component {
    Parameters::Parameters(int num_beams, float speed, int laser_scanner_interval)
    {
        _num_beams = num_beams;
        _speed = speed;
        _laser_scanner_interval = laser_scanner_interval;
    }
    void Parameters::render()
    {
        if (ImGui::CollapsingHeader("Parameter", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("#beams", &_num_beams, 1, 1000);
            ImGui::SliderFloat("Speed", &_speed, 0, 1);
            ImGui::SliderInt("Laser scanner interval", &_laser_scanner_interval, 1, 100);
        }
    }
}
}