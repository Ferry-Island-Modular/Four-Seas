
#pragma once
#include <stdint.h>

namespace fourseas
{
/**
    @brief Hardware Interface for control inputs with configurable bit depth
    Primarily designed for ADC input controls such as
    potentiometers, and control voltage.
*/
template <typename T, typename RawType>
class AnalogControlGeneric
{
  public:
    /** Constructor */
    AnalogControlGeneric() {}
    /** destructor */
    ~AnalogControlGeneric() {}

    /** 
    Initializes the control
    \param *adcptr is a pointer to the raw adc read value
    \param sr is the samplerate in Hz that the Process function will be called at.
    \param flip determines whether the input is flipped (i.e. 1.f - input) or not before being processed.
    \param invert determines whether the input is inverted (i.e. -1.f * input) or not before being processed.
    \param slew_seconds is the slew time in seconds that it takes for the control to change to a new value.
    */
    void Init(RawType *adcptr,
              float    sr,
              bool     flip         = false,
              bool     invert       = false,
              float    slew_seconds = 0.002f)
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

    /** 
    This Initializes the AnalogControl for a -5V to 5V inverted input
    All of the Init details are the same otherwise
    \param *adcptr Pointer to analog digital converter
    \param sr Audio engine sample rate
    */
    void InitBipolarCv(RawType *adcptr, float sr, float slew_seconds = 0.002f)
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

    /** 
    Filters, and transforms a raw ADC read into a normalized range.
    */
    float Process()
    {
        float t;
        t = static_cast<T *>(this)->RawToFloat(*raw_);
        if(flip_)
            t = 1.f - t;
        t = (t - offset_) * scale_ * (invert_ ? -1.0f : 1.0f);
        val_ += coeff_ * (t - val_);
        return val_;
    }

    /** Returns the current stored value, without reprocessing */
    inline float Value() const { return val_; }

    /** Set the Coefficient of the one pole smoothing filter */
    inline void SetCoeff(float val)
    {
        val    = val > 1.f ? 1.f : val;
        val    = val < 0.f ? 0.f : val;
        coeff_ = val;
    }

    /** Returns the raw value from the ADC */
    inline RawType GetRawValue() { return *raw_; }

    /** Returns a normalized float value representing the current ADC value. */
    inline float GetRawFloat()
    {
        return static_cast<T *>(this)->RawToFloat(*raw_);
    }

    /** Set a new sample rate after the ctrl has been initialized */
    void SetSampleRate(float sample_rate)
    {
        samplerate_ = sample_rate;
        float slew  = is_bipolar_ ? .002f : slew_seconds_;
        SetCoeff(1.0f / (slew * samplerate_ * 0.5f));
    }

    /** Set scale factor */
    void SetScale(float scale) { scale_ = scale; }

    /** Set offset value */
    void SetOffset(float offset) { offset_ = offset; }

    /** Get scale factor */
    float GetScale() { return scale_; }

    /** Get offset value */
    float GetOffset() { return offset_; }

    /** Check if control is bipolar */
    inline bool isBipolar() { return is_bipolar_; }

  protected:
    float scale_, offset_;

  private:
    RawType *raw_;
    float    coeff_, samplerate_, val_;
    bool     flip_;
    bool     invert_;
    bool     is_bipolar_;
    float    slew_seconds_;
};

// 16-bit ADC control (from daisy)
class AnalogControl16 : public AnalogControlGeneric<AnalogControl16, uint16_t>
{
  public:
    /** Convert raw ADC value to normalized float */
    float RawToFloat(uint16_t raw)
    {
        return static_cast<float>(raw) / 65535.0f;
    }
};

// 24-bit ADC control
class AnalogControl24 : public AnalogControlGeneric<AnalogControl24, uint32_t>
{
  public:
    /** Convert raw ADC value to normalized float */
    float RawToFloat(uint32_t raw)
    {
        return static_cast<float>(raw) / 8388607.0f;
    }
};

// For compatibility with existing code
using AnalogControl = AnalogControl16;

} // namespace fourseas