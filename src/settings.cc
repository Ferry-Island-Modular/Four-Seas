#include "daisy.h"
#include "settings.h"

using namespace fourseas;

void Settings::Init(SettingsData data)
{
    data_ = data;
}

void Settings::Save() {}

SettingsData Settings::Default()
{
    CalibrationData default_calibration_data;

    default_calibration_data.pitch_offset = 0.0f;
    default_calibration_data.pitch_scale  = 1.2f;
    default_calibration_data.bank_offset  = 0.0f;

    for(size_t i = 0; i < FourSeasHW::KNOB_LAST; i++)
    {
        default_calibration_data.knob_offsets[i] = 0.0f;
        default_calibration_data.knob_scales[i]  = 1.0f;
    }

    for(size_t i = 0; i < FourSeasHW::ADC_CV_LAST; i++)
    {
        default_calibration_data.adc_cv_offsets[i] = 0.5f;
    }

    for(size_t i = 0; i < CalibrationData::NUM_SWITCH_POSITIONS; i++)
    {
        default_calibration_data.rotary_switch_thresholds[i] = 0.0f;
    }


    SettingsData default_data = {default_calibration_data};

    return default_data;
}

CalibrationData* Settings::GetCalibrationData()
{
    return &data_.calData;
}
