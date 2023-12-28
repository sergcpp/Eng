#pragma once

#include <Ren/Log.h>

namespace Eng {
class FlowControl {
    enum class eMode { Good, Bad };
    eMode mode_;
    float penalty_time_, good_conditions_time_, penalty_reduction_acc_;

    unsigned int bad_delta_, good_delta_;

  public:
    FlowControl(unsigned int bad_delta, unsigned int good_delta);

    void Reset();

    void Update(float dt_s, float rtt, Ren::ILog *log);

    unsigned int send_period() { return mode_ == eMode::Good ? good_delta_ : bad_delta_; }
};
} // namespace Eng