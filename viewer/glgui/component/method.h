#pragma once

namespace glgui {
namespace component {
    class Method {
    public:
        bool _odometry_enabled;
        bool _scan_matching_enabled;
        Method();
        void render();
    };
}
}