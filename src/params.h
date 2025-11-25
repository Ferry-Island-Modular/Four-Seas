#pragma once

#include "stmlib/dsp/parameter_interpolator.h"

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
} // namespace fourseas