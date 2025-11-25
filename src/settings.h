#pragma once

#include "src/hardware/fourSeasBoard.h"

namespace fourseas
{
struct CalibrationData
{
    float pitch_offset;
    float pitch_scale;
    float bank_offset;
    float knob_scales[FourSeasHW::KNOB_LAST];

    float adc_cv_offsets[FourSeasHW::ADC_CV_LAST];
    float knob_offsets[FourSeasHW::KNOB_LAST];

    static constexpr uint8_t NUM_SWITCH_POSITIONS
        = FourSeasHW::kCurrentBoardRevVar == FourSeasHW::BoardRevision::REV_3
              ? 5
              : 8;

    float rotary_switch_thresholds[NUM_SWITCH_POSITIONS];
};

inline bool operator==(const CalibrationData& lhs, const CalibrationData& rhs)
{
    for(size_t i = 0; i < FourSeasHW::ADC_CV_LAST; i++)
    {
        if(lhs.adc_cv_offsets[i] != rhs.adc_cv_offsets[i])
        {
            return false;
        }
    }

    for(size_t i = 0; i < FourSeasHW::KNOB_LAST; i++)
    {
        if(lhs.knob_offsets[i] != rhs.knob_offsets[i])
        {
            return false;
        }

        if(lhs.knob_scales[i] != rhs.knob_scales[i])
        {
            return false;
        }
    }

    for(size_t i = 0; i < CalibrationData::NUM_SWITCH_POSITIONS; i++)
    {
        if(lhs.rotary_switch_thresholds[i] != rhs.rotary_switch_thresholds[i])
        {
            return false;
        }
    }

    if(lhs.pitch_offset != rhs.pitch_offset)
    {
        return false;
    }
    if(lhs.pitch_scale != rhs.pitch_scale)
    {
        return false;
    }
    if(lhs.bank_offset != rhs.bank_offset)
    {
        return false;
    }

    return true;
}

inline bool operator!=(const CalibrationData& lhs, const CalibrationData& rhs)
{
    return !(lhs == rhs);
}


struct SettingsData
{
    CalibrationData calData;
};

inline bool operator==(const SettingsData& lhs, const SettingsData& rhs)
{
    // TODO: double-check when we do calibration for real
    return lhs.calData == rhs.calData;
}

inline bool operator!=(const SettingsData& lhs, const SettingsData& rhs)
{
    return !(lhs == rhs);
}

class Settings
{
  public:
    enum CalState
    {
        CALIBRATION_LOW,
        CALIBRATION_HIGH,
        CALIBRATION_C1,
        CALIBRATION_C3,
        CALIBRATION_SPREAD_SWITCH,
        CALIBRATION_LAST
    };

    Settings() {}
    ~Settings() {}

    void             Init(SettingsData data);
    void             Save();
    CalibrationData* GetCalibrationData();

    SettingsData Default();

    inline bool operator!=(const Settings& a) { return a.data_ != data_; }

  private:
    SettingsData data_;
};

} // namespace fourseas
