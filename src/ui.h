#pragma once

#include <array>

#include "daisy.h"
#include "src/app_state.h"
#include "src/hardware/fourSeasBoard.h"
#include "src/parameter_24.h"
#include "src/params.h"
#include "src/settings.h"

#include "stmlib/stmlib.h"
#include "daisysp.h"

namespace fourseas
{
constexpr size_t NUM_RGB_LEDS = 12;

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

struct OscillatorParams
{
    const Params::Values& values;
    bool                  interpolate;
    uint8_t               mod_state  = 0;
    float                 mod_input  = 0.0f;
    uint8_t               sync_state = 0;
    bool                  sync_input = false;
};

// amount must be between 0.0 and 1.0
constexpr inline uint8_t crossfade(uint8_t a, uint8_t b, float amount)
{
    // R = a + i(b - a)
    return static_cast<uint8_t>(a + (amount * (b - a)));
}

class Ui
{
  public:
    enum WT_OSCS
    {
        WT_AUDIO_1,
        WT_AUDIO_2,
        WT_AUDIO_3,
        WT_AUDIO_4,
        WT_OSCS_LAST
    };

    enum DSY_PARAMS
    {
        POT_X,
        POT_Y,
        POT_Z,

        POT_X_SPREAD,
        POT_Y_SPREAD,
        POT_Z_SPREAD,

        POT_X_ATT,
        POT_Y_ATT,
        POT_Z_ATT,

        POT_TUNING_COARSE,
        POT_TUNING_FINE,
        POT_FM_ATT,
        POT_OSC_MOD_DEPTH_1,
        POT_OSC_MOD_DEPTH_2,
        POT_FREQ_SPREAD,
        POT_FREQ_SPREAD_ATT,

        CV_VOCT,
        CV_BANK,
        SPREAD_SWITCH,
        POT_BANK,

        DSY_PARAM_LAST,
    };

    enum ADC_CVS
    {
        CV_Z_POSITION,
        CV_Y_POSITION,
        CV_X_POSITION,
        CV_Z_SPREAD,
        CV_FM,
        CV_TUNING_SPREAD,
        CV_X_SPREAD,
        CV_Y_SPREAD,

        CV_LAST,
    };

    enum DIR_CVS
    {
        CV_VOCT_D,
        CV_BANK_D,
        CV_LAST_D
    };


    enum BUTTONS
    {
        SW_TUNING_LOCK,
        SW_LFO_TOGGLE_2,
        SW_OSC_SYNC_TYPE_1,
        SW_OSC_SYNC_TYPE_2,
        SW_WAVE_INTERPOLATE_TOGGLE,
        SW_LFO_TOGGLE_1,
        SW_OSC_MODE_1,
        SW_OSC_MODE_2,

        SW_LAST,
    };

    enum
    {
        SYNC_1,
        SYNC_2,

        SYNC_LAST,
    };

    enum LEDs
    {
        // IC1, 0x0
        LED_R_Z_POS,
        LED_G_Y_POS,
        LED_B_Y_POS,
        LED_R_Y_POS,

        LED_G_X_POS,
        LED_B_X_POS,
        LED_R_X_POS,
        LED_TUNING_LOCK,

        LED_G_Y_SPREAD,
        LED_R_Y_SPREAD,
        LED_B_Y_SPREAD,
        LED_G_OSC_SYNC_TYPE_1,

        LED_B_OSC_SYNC_TYPE_1,
        LED_R_OSC_SYNC_TYPE_1,
        LED_G_Z_POS,
        LED_B_Z_POS,

        // IC2, 0x1
        LED_WAVE_INTERPOLATE_TOGGLE,
        LED_G_OSC_MODE_1,
        LED_B_OSC_MODE_1,
        LED_R_OSC_MODE_1,

        // xxx = 20
        LED_G_OSC_MODE_2 = 21,
        LED_B_OSC_MODE_2,
        LED_R_OSC_MODE_2,

        LED_R_Z_SPREAD,
        LED_G_Z_SPREAD,
        LED_B_Z_SPREAD,
        LED_LFO_1_TOGGLE,

        LED_LFO_2_TOGGLE,
        LED_B_OSC_SYNC_TYPE_2,
        LED_G_OSC_SYNC_TYPE_2,
        LED_R_OSC_SYNC_TYPE_2,

        // IC3, 0x2
        // xxx = 32
        // xxx = 33
        LED_G_FREQ_SPREAD = 34,
        LED_B_FREQ_SPREAD,

        LED_R_FREQ_SPREAD,
        LED_G_FM_AMT,
        LED_B_FM_AMT,
        LED_R_FM_AMT,

        // xxx = 40
        // xxx = 41
        // xxx = 42
        // xxx = 43

        // xxx = 44
        LED_G_X_SPREAD = 45,
        LED_B_X_SPREAD,
        LED_R_X_SPREAD,

        LED_LAST,
    };

    static constexpr std::array<LEDs, NUM_RGB_LEDS> red_leds_ = {
        LED_R_FM_AMT,
        LED_R_FREQ_SPREAD,
        LED_R_OSC_MODE_1,
        LED_R_OSC_MODE_2,
        LED_R_OSC_SYNC_TYPE_1,
        LED_R_OSC_SYNC_TYPE_2,
        LED_R_X_POS,
        LED_R_X_SPREAD,
        LED_R_Y_POS,
        LED_R_Y_SPREAD,
        LED_R_Z_POS,
        LED_R_Z_SPREAD,
    };
    static constexpr std::array<LEDs, NUM_RGB_LEDS> green_leds_ = {
        LED_G_FM_AMT,
        LED_G_FREQ_SPREAD,
        LED_G_OSC_MODE_1,
        LED_G_OSC_MODE_2,
        LED_G_OSC_SYNC_TYPE_1,
        LED_G_OSC_SYNC_TYPE_2,
        LED_G_X_POS,
        LED_G_X_SPREAD,
        LED_G_Y_POS,
        LED_G_Y_SPREAD,
        LED_G_Z_POS,
        LED_G_Z_SPREAD,
    };

    static constexpr std::array<LEDs, NUM_RGB_LEDS> blue_leds_ = {
        LED_B_FM_AMT,
        LED_B_FREQ_SPREAD,
        LED_B_OSC_MODE_1,
        LED_B_OSC_MODE_2,
        LED_B_OSC_SYNC_TYPE_1,
        LED_B_OSC_SYNC_TYPE_2,
        LED_B_X_POS,
        LED_B_X_SPREAD,
        LED_B_Y_POS,
        LED_B_Y_SPREAD,
        LED_B_Z_POS,
        LED_B_Z_SPREAD,
    };

    Ui() {}
    ~Ui() {}

    void           Init(FourSeasHW*                             hw,
                        daisy::PersistentStorage<SettingsData>* settingsStorage,
                        daisy::PersistentStorage<AppState>*     appStateStorage);
    void           Calibrate();
    bool           Process();
    void           UpdateParams();
    Params::Values GetParams(size_t idx);
    uint8_t        GetBankNum();
    void           SetBanksMax(uint8_t bank_num);
    void           SetWavesLoaded(bool loaded);

    void UpdateLEDs();
    void SetLEDsRed();
    void SetLEDsGreen();
    void SetLEDsBlue();
    void SetLEDsPurple();
    void SetLEDsOff();
    void SetLEDsOn();
    void SetLEDsByValue(uint8_t val);
    void SetLEDsByChannel(const std::array<fourseas::Ui::LEDs, 12> chan,
                          uint8_t                                  val);
    bool isButtonPressed(uint8_t idx);
    bool isButtonReleased(uint8_t idx);
    bool isButtonUp(uint8_t idx);
    bool isButtonDown(uint8_t idx);
    void SetSpreadType(float switch_mode);
    void StartupLEDSequence();
    void FadeBankLED(size_t bank_index);
    void FadeToColor(uint8_t  target_r,
                     uint8_t  target_g,
                     uint8_t  target_b,
                     uint32_t duration_ms);

    Parameter24      cvs[CV_LAST];
    daisy::Parameter params[DSY_PARAM_LAST];
    daisy::Parameter bank_cv;

    AppState*              state_;
    daisy::VoctCalibration vcal_;

  private:
    float calculateSpread(size_t idx, float freq, float spread);

    FourSeasHW*            hw_;
    float                  freq_pots_;
    Params                 synth_params_[4];
    uint8_t                max_banks_ = 1;
    uint8_t                bank_num_;
    AppState::SPREAD_TYPES spread_type_;
    bool                   lock_tuning_;
    bool                   waves_loaded_;


    static constexpr std::array<std::array<LEDs, 3>, NUM_RGB_LEDS> bank_leds_
        = {{
            {{LED_R_FM_AMT, LED_G_FM_AMT, LED_B_FM_AMT}}, // Bank 1
            {{LED_R_FREQ_SPREAD,
              LED_G_FREQ_SPREAD,
              LED_B_FREQ_SPREAD}},                              // Bank 2
            {{LED_R_X_SPREAD, LED_G_X_SPREAD, LED_B_X_SPREAD}}, // Bank 3
            {{LED_R_X_POS, LED_G_X_POS, LED_B_X_POS}},          // Bank 4
            {{LED_R_Y_POS, LED_G_Y_POS, LED_B_Y_POS}},          // Bank 5
            {{LED_R_Y_SPREAD, LED_G_Y_SPREAD, LED_B_Y_SPREAD}}, // Bank 6
            {{LED_R_Z_POS, LED_G_Z_POS, LED_B_Z_POS}},          // Bank 7
            {{LED_R_Z_SPREAD, LED_G_Z_SPREAD, LED_B_Z_SPREAD}}, // Bank 8

            {{LED_R_OSC_SYNC_TYPE_1,
              LED_G_OSC_SYNC_TYPE_1,
              LED_B_OSC_SYNC_TYPE_1}}, // Bank 9

            {{LED_R_OSC_SYNC_TYPE_2,
              LED_G_OSC_SYNC_TYPE_2,
              LED_B_OSC_SYNC_TYPE_2}}, // Bank 10

            {{LED_R_OSC_MODE_2, LED_G_OSC_MODE_2, LED_B_OSC_MODE_2}}, // Bank 11
            {{LED_R_OSC_MODE_1, LED_G_OSC_MODE_1, LED_B_OSC_MODE_1}}, // Bank 12
        }};

    CalibrationData*                        calData_;
    daisy::PersistentStorage<SettingsData>* settingsStorage_;
    daisy::PersistentStorage<AppState>*     appStateStorage_;
};

} // namespace fourseas