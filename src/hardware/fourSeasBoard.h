#pragma once

#ifndef UNIT_TEST // for unit tests, a dummy implementation is provided below
#include "stm32h7xx_hal.h"
#include "daisy.h"
#include "daisy_seed.h"
#endif // ifndef UNIT_TEST

#include "src/button.h"
#include "src/analog_ctrl_24.h"
#include "src/cal_input.h"

#include "src/drivers/MCP23008.h"
#include "src/drivers/MCP3564R.h"
#include "src/drivers/TLC59116.h"

namespace fourseas
{
class FourSeasHW
{
  public:
    // Hardware version
    enum class BoardRevision
    {
        REV_3,
        REV_4,

        REV_LAST
    };

    enum AV_KNOBS
    {
        KNOB_0,
        KNOB_1,
        KNOB_2,
        KNOB_3,
        KNOB_4,
        KNOB_5,
        KNOB_6,
        KNOB_7,
        KNOB_8,
        KNOB_9,
        KNOB_10,
        KNOB_11,
        KNOB_12,
        KNOB_13,
        KNOB_14,
        KNOB_15,
        KNOB_LAST
    };

    enum ANALOG_INS
    {
        MUX_0,
        MUX_1,
        CV_0,
        CV_1,
        ROTARY_SWITCH,
        KNOB_DIRECT,

        ANALOG_LAST,
    };

    enum ADC_CVS
    {
        ADC_CV_0,
        ADC_CV_1,
        ADC_CV_2,
        ADC_CV_3,
        ADC_CV_4,
        ADC_CV_5,
        ADC_CV_6,
        ADC_CV_7,
        ADC_CV_LAST
    };

    enum ONBOARD_CVS
    {
        ONBOARD_CV_0,
        ONBOARD_CV_1,
        ONBOARD_CV_LAST
    };

    enum SR_BUTTONS
    {
        BUTTON_0,
        BUTTON_1,
        BUTTON_2,
        BUTTON_3,
        BUTTON_4,
        BUTTON_5,
        BUTTON_6,
        BUTTON_7,
        BUTTON_LAST
    };

    FourSeasHW() {}
    ~FourSeasHW() {}

    void Init(bool boost = false);
    void DelayMs(size_t del);
    void StartAudio(daisy::AudioHandle::InterleavingAudioCallback cb);
    void StartAudio(daisy::AudioHandle::AudioCallback cb);
    void ChangeAudioCallback(daisy::AudioHandle::InterleavingAudioCallback cb);
    void ChangeAudioCallback(daisy::AudioHandle::AudioCallback cb);
    void StopAudio();
    void SetAudioBlockSize(size_t size);
    size_t AudioBlockSize();
    void   SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate samplerate);
    float  AudioSampleRate();
    float  AudioCallbackRate();
    void   StartAdc();
    void   StopAdc();
    void   ProcessAnalogControls();
    void   InitLEDDriver();
    void   InitExtGPIO();
    void   UpdateLEDs();
    void   UpdateExtGPIO();
    uint32_t GetTick();
    uint32_t GetNow();
    void     InitWatchdog();
    void     RefreshWatchdog();

    daisy::SpiHandle::Result UpdateExtADC();

    TLC59116                    led_driver;
    daisy::DaisySeed            seed;
    fourseas::CalibratedControl knobs[KNOB_LAST];
    fourseas::CalibratedControl cv[2];
    fourseas::CalibratedControl extra_knob;
    fourseas::CalibratedControl rotary_switch;
    AnalogControl24             adc_cvs[ADC_CV_LAST];
    Button                      buttons[BUTTON_LAST];
    // temp, should probably be private
    AdcMCP3564R      adc_;
    daisy::I2CHandle ext_i2c_handle_;


    // Provide board revision for compile-time configuration
    static constexpr FourSeasHW::BoardRevision kCurrentBoardRevVar =
#ifdef CURRENT_BOARD_REV
        CURRENT_BOARD_REV;
#else
        FourSeasHW::BoardRevision::REV_4; // Default if not specified
#endif

  private:
    void InitAnalogInputs();
    void SetHidUpdateRates();
    void InitExtADC();
    void InitADCCVs();
    void InitAudio();
    void InitExtI2C();

    MCP23008           ext_gpio_;
    dsy_gpio           adc_irq_pin_;
    IWDG_HandleTypeDef hiwdg_;
};

} // namespace fourseas
