#include "src/analog_ctrl_24.h"
#include <math.h>

using namespace fourseas;

void AnalogControl24::Init(uint32_t *adcptr,
                           float     sr,
                           bool      flip,
                           bool      invert,
                           float     slew_seconds)
{
    val_        = 0.0f;
    raw_        = adcptr;
    samplerate_ = sr;
    SetCoeff(1.0f / (slew_seconds * samplerate_ * 0.5f));
    scale_        = 1.0f;
    offset_       = 0.0f;
    flip_         = flip;
    invert_       = invert;
    is_bipolar_   = false;
    slew_seconds_ = slew_seconds;
}

void AnalogControl24::InitBipolarCv(uint32_t *adcptr,
                                    float     sr,
                                    float     slew_seconds)
{
    val_        = 0.0f;
    raw_        = adcptr;
    samplerate_ = sr;
    SetCoeff(1.0f / (slew_seconds * samplerate_ * 0.5f));
    scale_      = 2.0f;
    offset_     = 0.5f;
    flip_       = false;
    invert_     = true;
    is_bipolar_ = true;
}

float AnalogControl24::Process()
{
    float t;
    t = (float)*raw_
        / 8388607.0f; // TODO: ensure this is correct and check sign bit
    if(flip_)
        t = 1.f - t;
    t = (t - offset_) * scale_ * (invert_ ? -1.0f : 1.0f);
    val_ += coeff_ * (t - val_);
    return val_;
}

void AnalogControl24::SetSampleRate(float sample_rate)
{
    samplerate_ = sample_rate;
    float slew  = is_bipolar_ ? .002f : slew_seconds_;
    SetCoeff(1.0f / (slew * samplerate_ * 0.5f));
}

void AnalogControl24::SetScale(float scale)
{
    scale_ = scale;
};

void AnalogControl24::SetOffset(float offset)
{
    offset_ = offset;
};

float AnalogControl24::GetScale()
{
    return scale_;
};

float AnalogControl24::GetOffset()
{
    return offset_;
};