#include "cal_input.h"

using namespace fourseas;

void CalibratedControl::SetScale(float scale)
{
    scale_ = scale;
};

void CalibratedControl::SetOffset(float offset)
{
    offset_ = offset;
};

float CalibratedControl::GetScale()
{
    return scale_;
};

float CalibratedControl::GetOffset()
{
    return offset_;
};
