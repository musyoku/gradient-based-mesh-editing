#pragma once

namespace glgui {
namespace component {
    class Noise {
    public:
        float _odometry_stddev;
        float _scan_stddev;
        Noise(float odometry_stddev, float scan_stddev);
        void render();
    };
}
}