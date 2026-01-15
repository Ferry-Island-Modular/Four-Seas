#pragma once

#include "stmlib/dsp/parameter_interpolator.h"
#include "daisysp.h"

namespace fourseas
{
class Params
{
  public:
    struct Values
    {
        float frequency;
        float x;
        float y;
        float z;
        float osc_mod_amount;
    };

    Params() {}
    ~Params() {}

    void   Init(size_t size);
    void   Update(Params::Values vals);
    Values Fetch();

  private:
    stmlib::ParameterInterpolator frequency_interpolator_;
    stmlib::ParameterInterpolator x_interpolator_;
    stmlib::ParameterInterpolator y_interpolator_;
    stmlib::ParameterInterpolator z_interpolator_;
    stmlib::ParameterInterpolator osc_mod_interpolator_;

    Values values_;

    size_t size_;
};

struct OscillatorParams
{
    const Params::Values& values;
    bool                  interpolate;
    uint8_t               mod_state  = 0;
    float                 mod_input  = 0.0f;
    uint8_t               sync_state = 0;
    bool                  sync_input = false;
};

// Utility function for applying dead zone to parameter values
inline float DeadZone(float value, float limit)
{
    float scaler = 1.0f / (1.0f - 2.0f * limit);

    if(value < limit * -1.0f)
    {
        value += limit;
        value *= scaler;
    }
    else if(value > limit)
    {
        value -= limit;
        value *= scaler;
    }
    else
    {
        value = 0.0f;
    }
    value = daisysp::fclamp(value, -1.0f, 1.0f);

    return value;
}

} // namespace fourseas
