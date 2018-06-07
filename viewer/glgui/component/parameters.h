#pragma once

namespace glgui {
namespace component {
    class Parameters {
    public:
        int _num_beams;
        float _speed;

        //オドメトリに対するレーザースキャナーの観測頻度
        //10にするとオドメトリによる位置予測を10回行うたびにレーザースキャンが1回走る
        int _laser_scanner_interval;
        Parameters(int num_beams, float speed, int laser_scanner_interval);
        void render();
    };
}
}