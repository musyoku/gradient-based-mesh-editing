#include "noise.h"
#include <external/imgui/imgui.h>

namespace glgui {
namespace component {
    Noise::Noise(float odometry_stddev, float scan_stddev)
    {
        _odometry_stddev = odometry_stddev;
        _scan_stddev = scan_stddev;
    }
    void Noise::render()
    {
        if (ImGui::CollapsingHeader("Noise", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Odometry##NoiseOdometry", &_odometry_stddev, 0, 0.1); // Odometoryというラベルはすでに使われているので適当なIDを付ける
            ImGui::SliderFloat("Scan", &_scan_stddev, 0, 0.01);
        }
    }
}
}