#pragma once

#include "hid/ctrl.h"

namespace fourseas
{
class CalibratedControl : public daisy::AnalogControl
{
  public:
    void SetScale(float scale);
    void SetOffset(float offset);

    float GetScale();
    float GetOffset();
};
} // namespace fourseas
