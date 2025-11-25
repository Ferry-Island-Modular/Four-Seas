#pragma once

#include <stdint.h>

namespace fourseas
{
class AnalogControl24
{
  public:
    AnalogControl24() {}
    ~AnalogControl24() {}

    /** 
    Initializes the control
    \param *adcptr is a pointer to the raw adc read value -- This can be acquired with dsy_adc_get_rawptr(), or dsy_adc_get_mux_rawptr()
    \param sr is the samplerate in Hz that the Process function will be called at.
    \param flip determines whether the input is flipped (i.e. 1.f - input) or not before being processed.1
    \param invert determines whether the input is inverted (i.e. -1.f * input) or note before being processed.
    \param slew_seconds is the slew time in seconds that it takes for the control to change to a new value.
    */
    void Init(uint32_t *adcptr,
              float     sr,
              bool      flip         = false,
              bool      invert       = false,
              float     slew_seconds = 0.002f);

    /** 
    This Initializes the AnalogControl for a -5V to 5V inverted input
    All of the Init details are the same otherwise
    \param *adcptr Pointer to analog digital converter
    \param sr Audio engine sample rate
    */
    void InitBipolarCv(uint32_t *adcptr, float sr, float slew_seconds = 0.002f);

    /** 
    Filters, and transforms a raw ADC read into a normalized range.
    this should be called at the rate of specified by samplerate at Init time.   
    Default Initializations will return 0.0 -> 1.0
    Bi-polar CV inputs will return -1.0 -> 1.0
    */
    float Process();

    /** Returns the current stored value, without reprocessing */
    inline float Value() const { return val_; }

    /** Directly set the Coefficient of the one pole smoothing filter. 
      \param val Value to set coefficient to. Max of 1, min of 0.
    */
    // using conditionals since clamp() is unavailable
    inline void SetCoeff(float val)
    {
        val = val > 1.f ? 1.f : val;
        val = val < 0.f ? 0.f : val;

        coeff_ = val;
    }

    /** Returns the raw unsigned 24-bit value from the ADC */
    inline uint32_t GetRawValue() { return *raw_; }

    /** Returns a normalized float value representing the current ADC value. */
    inline float GetRawFloat() { return (float)(*raw_) / 8388607.0f; }

    /** Set a new sample rate after the ctrl has been initialized
     *  \param sample_rate New update rate for the switch in hz
    */
    void SetSampleRate(float sample_rate);

    // Own calibration related methods
    void SetScale(float scale);
    void SetOffset(float offset);

    float GetScale();
    float GetOffset();

  protected:
    float scale_, offset_;

  private:
    uint32_t *raw_;
    float     coeff_, samplerate_, val_;
    bool      flip_;
    bool      invert_;
    bool      is_bipolar_;
    float     slew_seconds_;
};
} // namespace fourseas
