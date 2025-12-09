#include "stmlib/stmlib.h"
#include "daisysp.h"

#include "src/constants.h"
#include "src/hardware/fourSeasBoard.h"
#include "src/resources.h"
#include "src/ui.h"

namespace fourseas
{
uint32_t lastExecutionTime = 0; // Stores the last time the function was called
constexpr uint32_t intervalTicks = 1000 * 3; // ~3 seconds, experiment with this

static const float spread_coeffs[4] = {0.0f, 2.3333f, 4.6666f, 6.9999f};

static constexpr uint8_t LED_COLOR_ONE_R = 0;
static constexpr uint8_t LED_COLOR_ONE_G = 145;
static constexpr uint8_t LED_COLOR_ONE_B = 255;

static constexpr uint8_t LED_COLOR_TWO_R = 255;
static constexpr uint8_t LED_COLOR_TWO_G = 150;
static constexpr uint8_t LED_COLOR_TWO_B = 0;

// TODO: clean this up to init settings directly
__attribute__((optimize("Os"))) void
Ui::Init(FourSeasHW*                             hw,
         daisy::PersistentStorage<SettingsData>* settingsStorage,
         daisy::PersistentStorage<AppState>*     appStateStorage)
{
    hw_              = hw;
    settingsStorage_ = settingsStorage;
    appStateStorage_ = appStateStorage;

    Settings settings;
    settingsStorage_->Init(settings.Default());

    // Get local reference to settings struct
    SettingsData& data = settingsStorage_->GetSettings();
    calData_           = &data.calData;
    settings.Init(data);

    // Get local reference to settings struct
    state_ = &appStateStorage_->GetSettings();

    for(size_t i = 0; i < 4; i++)
    {
        synth_params_[i].Init(hw_->AudioBlockSize());
    }

    // Apply offsets from saved config
    for(size_t i = 0; i < hw_->ADC_CV_LAST; i++)
    {
        hw_->adc_cvs[i].SetOffset(calData_->adc_cv_offsets[i]);
    }

    for(size_t i = 0; i < hw_->KNOB_LAST; i++)
    {
        hw_->knobs[i].SetOffset(calData_->knob_offsets[i]);
    }

    // Coarse tuning knob
    hw_->knobs[hw_->KNOB_14].SetScale(calData_->knob_scales[hw_->KNOB_14]);

    // Freq spread knob
    hw_->knobs[hw_->KNOB_15].SetScale(calData_->knob_scales[hw_->KNOB_15]);

    // Apply scaling to v/oct in CV
    hw_->cv[hw_->ONBOARD_CV_0].SetScale(2.0);
    vcal_.SetData(calData_->pitch_scale, calData_->pitch_offset);

    // Apply offset to bank CV
    hw_->cv[hw_->ONBOARD_CV_1].SetOffset(calData_->bank_offset);

    // Init pots
    params[POT_TUNING_COARSE].Init(
        hw_->knobs[hw_->KNOB_14], 21.0f, 105.0f, daisy::Parameter::LINEAR);
    params[POT_TUNING_FINE].Init(
        hw_->knobs[hw_->KNOB_12], -3.5f, 3.5f, daisy::Parameter::LINEAR);
    params[POT_FM_ATT].Init(
        hw_->knobs[hw_->KNOB_13], -1.0f, 1.0f, daisy::Parameter::LINEAR);

    params[POT_X].Init(
        hw_->knobs[hw_->KNOB_11], 0.0f, 6.9999f, daisy::Parameter::LINEAR);
    params[POT_Y].Init(
        hw_->knobs[hw_->KNOB_4], 0.0f, 6.9999f, daisy::Parameter::LINEAR);
    params[POT_Z].Init(
        hw_->knobs[hw_->KNOB_3], 0.0f, 6.9999f, daisy::Parameter::LINEAR);

    params[POT_X_SPREAD].Init(
        hw_->knobs[hw_->KNOB_9], -1.0f, 1.0f, daisy::Parameter::LINEAR);
    params[POT_Y_SPREAD].Init(
        hw_->knobs[hw_->KNOB_6], -1.0f, 1.0f, daisy::Parameter::LINEAR);
    params[POT_Z_SPREAD].Init(
        hw_->knobs[hw_->KNOB_0], -1.0f, 1.0f, daisy::Parameter::LINEAR);

    params[POT_X_ATT].Init(
        hw_->knobs[hw_->KNOB_10], -1.0f, 1.0f, daisy::Parameter::LINEAR);
    params[POT_Y_ATT].Init(
        hw_->knobs[hw_->KNOB_7], -1.0f, 1.0f, daisy::Parameter::LINEAR);
    params[POT_Z_ATT].Init(
        hw_->knobs[hw_->KNOB_5], -1.0f, 1.0f, daisy::Parameter::LINEAR);

    params[POT_FREQ_SPREAD].Init(
        hw_->knobs[hw_->KNOB_15], -1.0f, 1.0f, daisy::Parameter::LINEAR);
    params[POT_FREQ_SPREAD_ATT].Init(
        hw_->knobs[hw_->KNOB_8], -1.0f, 1.0f, daisy::Parameter::LINEAR);

    params[POT_OSC_MOD_DEPTH_1].Init(
        hw_->knobs[hw_->KNOB_1], 0.0f, 1.0f, daisy::Parameter::LINEAR);
    params[POT_OSC_MOD_DEPTH_2].Init(
        hw_->knobs[hw_->KNOB_2], 0.0f, 1.0f, daisy::Parameter::LINEAR);

    params[POT_BANK].Init(
        hw_->extra_knob, 0.0f, 1.0f, daisy::Parameter::LINEAR);
    params[SPREAD_SWITCH].Init(
        hw_->rotary_switch, 0.0f, 1.0f, daisy::Parameter::LINEAR);

    // Init CV ins (are bi-polar, don't need negative minimum)
    cvs[CV_X_POSITION].Init(hw_->adc_cvs[hw_->ADC_CV_2],
                            0.0f,
                            6.9999f,
                            fourseas::Parameter24::LINEAR);
    cvs[CV_Y_POSITION].Init(hw_->adc_cvs[hw_->ADC_CV_1],
                            0.0f,
                            6.9999f,
                            fourseas::Parameter24::LINEAR);
    cvs[CV_Z_POSITION].Init(hw_->adc_cvs[hw_->ADC_CV_0],
                            0.0f,
                            6.9999f,
                            fourseas::Parameter24::LINEAR);

    cvs[CV_X_SPREAD].Init(
        hw_->adc_cvs[hw_->ADC_CV_7], 0.0f, 1.0f, fourseas::Parameter24::LINEAR);
    cvs[CV_Y_SPREAD].Init(
        hw_->adc_cvs[hw_->ADC_CV_6], 0.0f, 1.0f, fourseas::Parameter24::LINEAR);
    cvs[CV_Z_SPREAD].Init(
        hw_->adc_cvs[hw_->ADC_CV_3], 0.0f, 1.0f, fourseas::Parameter24::LINEAR);

    cvs[CV_TUNING_SPREAD].Init(
        hw_->adc_cvs[hw_->ADC_CV_5], 0.0f, 1.0f, fourseas::Parameter24::LINEAR);
    cvs[CV_FM].Init(hw_->adc_cvs[hw_->ADC_CV_4],
                    0.0f,
                    60.0f,
                    fourseas::Parameter24::LINEAR);

    bank_cv.Init(
        hw_->cv[hw_->ONBOARD_CV_1], 0.0f, 1.0f, daisy::Parameter::LINEAR);
}

__attribute__((optimize("Os"))) void Ui::Calibrate()
{
    SetLEDsOff();

    // 1. Launch calibration by pressing both osc mod buttons
    // Little light show to indicate cal start
    for(size_t i = 0; i < 3; i++)
    {
        SetLEDsOff();
        hw_->DelayMs(150);
        SetLEDsOn();
        hw_->DelayMs(150);
    }

    Settings::CalState cal_state = Settings::CALIBRATION_LOW;

    float v1 = 0.0f;
    float v3 = 0.0f;

    SetLEDsOff();

    while(cal_state < Settings::CALIBRATION_LAST)
    {
        hw_->RefreshWatchdog();

        hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_2, 255);
        hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_2, 255);
        hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_2, 255);

        switch(cal_state)
        {
            // 2. Normalize pots and CV (LEDs all red)
            // Reset CVs and knobs to remove any offset
            // Unplug all CVs and turn knobs all the way down
            // Press sync type 2 button when ready

            // v/oct offset should *not* be calculated with v1 plugged in
            case Settings::CALIBRATION_LOW:
            {
                SetLEDsRed();

                for(size_t i = 0; i < hw_->ADC_CV_LAST; i++)
                {
                    calData_->adc_cv_offsets[i]
                        = (hw_->adc_cvs[i].GetRawFloat()
                           + calData_->adc_cv_offsets[i])
                          / 2.0f;
                }

                // Set knob offsets
                for(size_t i = 0; i < hw_->KNOB_LAST; i++)
                {
                    calData_->knob_offsets[i] = (hw_->knobs[i].GetRawFloat()
                                                 + calData_->knob_offsets[i])
                                                / 2.0f;
                }

                calData_->pitch_offset
                    = (hw_->cv[hw_->ONBOARD_CV_0].GetRawFloat()
                       + calData_->pitch_offset)
                      / 2.0f;

                // Bank CV offset calibration
                calData_->bank_offset
                    = (hw_->cv[hw_->ONBOARD_CV_1].GetRawFloat()
                       + calData_->bank_offset)
                      / 2.0f;

                if(isButtonReleased(SW_OSC_SYNC_TYPE_2))
                {
                    SetLEDsOff();
                    cal_state = Settings::CALIBRATION_HIGH;
                }
                break;
            }
            // 3. Ceiling for certain pots/CV (LEDs all green)

            // Sets scaling relevant knobs and CVs
            // Frequency spread needs this at least to properly track
            // POT_FREQ_SPREAD_ATT and POT_FM_ATT as well
            // i.e., ratios should be precise when knob is all the way up
            // TODO: how should this track for CV?
            // Turn frequency spread and tuning coarse knobs all the way up
            // Press sync type 2 button when ready
            case Settings::CALIBRATION_HIGH:
            {
                SetLEDsGreen();

                if(isButtonReleased(SW_OSC_SYNC_TYPE_2))
                {
                    // Coarse tuning knob
                    calData_->knob_scales[hw_->KNOB_14]
                        = 1.0f
                          / (hw_->knobs[hw_->KNOB_14].GetRawFloat()
                             - calData_->knob_offsets[hw_->KNOB_14]);

                    // Freq spread knob
                    calData_->knob_scales[hw_->KNOB_15]
                        = 1.0f
                          / (hw_->knobs[hw_->KNOB_15].GetRawFloat()
                             - calData_->knob_offsets[hw_->KNOB_15]);

                    SetLEDsOff();
                    cal_state = Settings::CALIBRATION_C1;
                }
                break;
            }

            // 4. 1/V octave calibration step 1 (LEDs all blue)
            // Plug 1.0V DC CV signal into 1 V/OCT input
            // Press sync type 2 button when ready
            case Settings::CALIBRATION_C1:
            {
                SetLEDsBlue();

                v1 = hw_->cv[hw_->ONBOARD_CV_0].Process();

                if(isButtonReleased(SW_OSC_SYNC_TYPE_2))
                {
                    SetLEDsOff();
                    cal_state = Settings::CALIBRATION_C3;
                }
                break;
            }

            // 5. 1/V octave calibration step 2 (LEDs all white)
            // Plug 3.0V DC CV signal into 1 V/OCT input
            // Press sync type 2 button when ready
            case Settings::CALIBRATION_C3:
            {
                SetLEDsPurple();

                v3 = hw_->cv[hw_->ONBOARD_CV_0].Process();

                if(isButtonReleased(SW_OSC_SYNC_TYPE_2))
                {
                    SetLEDsOff();
                    cal_state = Settings::CALIBRATION_SPREAD_SWITCH;
                }
                break;
            }

            // 6. Spread mode switch thresholds
            // Press sync type 2 button when ready
            case Settings::CALIBRATION_SPREAD_SWITCH:
            {
                uint8_t i = 0;
                while(i < CalibrationData::NUM_SWITCH_POSITIONS)
                {
                    hw_->RefreshWatchdog();

                    float spr = params[SPREAD_SWITCH].Process();
                    calData_->rotary_switch_thresholds[i] = spr;

                    if(isButtonReleased(SW_OSC_SYNC_TYPE_2))
                    {
                        i++;
                        hw_->led_driver.Set(bank_leds_[i][0], 255);
                        hw_->led_driver.Set(bank_leds_[i][1], 255);
                        hw_->led_driver.Set(bank_leds_[i][2], 255);
                        hw_->UpdateLEDs();
                    }
                }

                // After collecting all thresholds, calculate the midpoints for actual thresholds
                for(uint8_t j = 0;
                    j < CalibrationData::NUM_SWITCH_POSITIONS - 1;
                    j++)
                {
                    calData_->rotary_switch_thresholds[j]
                        = (calData_->rotary_switch_thresholds[j]
                           + calData_->rotary_switch_thresholds[j + 1])
                          / 2.0f;
                }

                SetLEDsOff();
                cal_state = Settings::CALIBRATION_LAST;
                break;
            }
            default: break;
        }
    }
    // 6. Calibration routine finished, resume normal operation
    // v1 = C3, v3 = C5
    vcal_.Record(v1, v3);
    vcal_.GetData(calData_->pitch_scale, calData_->pitch_offset);

    settingsStorage_->Save();
}

// Returns true if calibration routine was executed. Seems like there should be a more elegant way
bool Ui::Process()
{
    if(isButtonDown(SW_OSC_MODE_1) && isButtonDown(SW_OSC_MODE_2))
    {
        if(hw_->buttons[SW_OSC_MODE_1].TimeHeldMs() > 5000
           && hw_->buttons[SW_OSC_MODE_1].TimeHeldMs() > 5000)
        {
            Calibrate();
            return true;
        }
    }

    // interpolate_waves
    if(isButtonReleased(SW_WAVE_INTERPOLATE_TOGGLE))
    {
        state_->interpolate_waves = !state_->interpolate_waves;
    }

    // lock_tuning
    if(isButtonReleased(SW_TUNING_LOCK))
    {
        lock_tuning_ = !lock_tuning_;
    }

    if(isButtonReleased(SW_OSC_MODE_1))
    {
        switch(state_->mod_state_1)
        {
            case AppState::MOD_STATES::PHASE_MOD:
            {
                state_->mod_state_1 = AppState::MOD_STATES::WAVESHAPING;
                break;
            }

            case AppState::MOD_STATES::WAVESHAPING:
            {
                state_->mod_state_1 = AppState::MOD_STATES::XOR;
                break;
            }

            case AppState::MOD_STATES::XOR:
            {
                state_->mod_state_1 = AppState::MOD_STATES::PHASE_MOD;
                break;
            }

            default: break;
        }
    }

    if(isButtonReleased(SW_OSC_MODE_2))
    {
        switch(state_->mod_state_2)
        {
            case AppState::MOD_STATES::PHASE_MOD:
            {
                state_->mod_state_2 = AppState::MOD_STATES::WAVESHAPING;
                break;
            }

            case AppState::MOD_STATES::WAVESHAPING:
            {
                state_->mod_state_2 = AppState::MOD_STATES::XOR;
                break;
            }

            case AppState::MOD_STATES::XOR:
            {
                state_->mod_state_2 = AppState::MOD_STATES::PHASE_MOD;
                break;
            }

            default: break;
        }
    }


    // LFO modes
    if(isButtonReleased(SW_LFO_TOGGLE_1))
    {
        state_->lfo_state_1 = !state_->lfo_state_1;
    }

    if(isButtonReleased(SW_LFO_TOGGLE_2))
    {
        state_->lfo_state_2 = !state_->lfo_state_2;
    }

    if(isButtonReleased(SW_OSC_SYNC_TYPE_1))
    {
        switch(state_->sync_mode_1)
        {
            case AppState::SYNC_MODES::HARD:
            {
                state_->sync_mode_1 = AppState::SYNC_MODES::SOFT;
                break;
            }

            case AppState::SYNC_MODES::SOFT:
            {
                state_->sync_mode_1 = AppState::SYNC_MODES::FLIP;
                break;
            }

            case AppState::SYNC_MODES::FLIP:
            {
                state_->sync_mode_1 = AppState::SYNC_MODES::HARD;
                break;
            }

            default: break;
        }
    }

    if(isButtonReleased(SW_OSC_SYNC_TYPE_2))
    {
        switch(state_->sync_mode_2)
        {
            case AppState::SYNC_MODES::HARD:
            {
                state_->sync_mode_2 = AppState::SYNC_MODES::SOFT;
                break;
            }

            case AppState::SYNC_MODES::SOFT:
            {
                state_->sync_mode_2 = AppState::SYNC_MODES::FLIP;
                break;
            }

            case AppState::SYNC_MODES::FLIP:
            {
                state_->sync_mode_2 = AppState::SYNC_MODES::HARD;
                break;
            }

            default: break;
        }
    }

    float switch_mode = params[SPREAD_SWITCH].Process();
    SetSpreadType(switch_mode);

    float b_tot = (params[POT_BANK].Process() + bank_cv.Process())
                  * static_cast<float>(max_banks_);
    b_tot = daisysp::fclamp(b_tot, 0.0f, static_cast<float>(max_banks_ - 1));
    bank_num_ = static_cast<uint8_t>(b_tot);

    UpdateLEDs();

    uint32_t currentTime = hw_->GetNow();

    if((currentTime - lastExecutionTime) >= intervalTicks)
    {
        appStateStorage_->Save();
        lastExecutionTime = currentTime;
    }

    return false;
}


void Ui::SetSpreadType(float switch_mode)
{
    // Loop through thresholds
    for(uint8_t i = 0; i < CalibrationData::NUM_SWITCH_POSITIONS - 1; i++)
    {
        if(switch_mode < calData_->rotary_switch_thresholds[i])
        {
            spread_type_ = static_cast<AppState::SPREAD_TYPES>(i);
            return;
        }
    }
    if constexpr(FourSeasHW::kCurrentBoardRevVar
                 == FourSeasHW::BoardRevision::REV_3)
    {
        spread_type_ = AppState::SPREAD_TYPES::SPREAD_FIVE;
    }
    else
    {
        spread_type_ = AppState::SPREAD_TYPES::SPREAD_EIGHT;
    }
}

void Ui::UpdateParams()
{
    Params::Values vals;

    float freq_val = 0.0f;

    // Update tuning pots if not locked
    if(!lock_tuning_)
    {
        freq_pots_ = params[POT_TUNING_COARSE].Process()
                     + params[POT_TUNING_FINE].Process();
    }
    freq_val = freq_pots_
               + vcal_.ProcessInput(hw_->cv[hw_->ONBOARD_CV_0].Process())
               + (cvs[CV_FM].Process() * params[POT_FM_ATT].Process());

    freq_val = daisysp::mtof(freq_val);

    // x, y, z positions
    float x_val
        = params[POT_X].Process()
          + (cvs[CV_X_POSITION].Process() * params[POT_X_ATT].Process());
    x_val = daisysp::fclamp(x_val, 0.0f, 6.9999f);

    float y_val
        = params[POT_Y].Process()
          + (cvs[CV_Y_POSITION].Process() * params[POT_Y_ATT].Process());
    y_val = daisysp::fclamp(y_val, 0.0f, 6.9999f);

    float z_val
        = params[POT_Z].Process()
          + (cvs[CV_Z_POSITION].Process() * params[POT_Z_ATT].Process());
    z_val = daisysp::fclamp(z_val, 0.0f, 6.9999f);

    // x, y, z spread amounts
    float x_spread_amt
        = params[POT_X_SPREAD].Process() + cvs[CV_X_SPREAD].Process();
    x_spread_amt = daisysp::fclamp(x_spread_amt, -1.0f, 1.0f);

    float y_spread_amt
        = params[POT_Y_SPREAD].Process() + cvs[CV_Y_SPREAD].Process();
    y_spread_amt = daisysp::fclamp(y_spread_amt, -1.0f, 1.0f);

    float z_spread_amt
        = params[POT_Z_SPREAD].Process() + cvs[CV_Z_SPREAD].Process();
    z_spread_amt = daisysp::fclamp(z_spread_amt, -1.0f, 1.0f);

    // Frequency spread
    float spread_val = params[POT_FREQ_SPREAD].Process()
                       + (cvs[CV_TUNING_SPREAD].Process()
                          * params[POT_FREQ_SPREAD_ATT].Process());
    spread_val = daisysp::fclamp(spread_val, -1.0f, 1.0f);

    float osc_mod_depth_pot_1 = params[POT_OSC_MOD_DEPTH_1].Process();
    float osc_mod_depth_pot_2 = params[POT_OSC_MOD_DEPTH_2].Process();

    for(size_t i = 0; i < 4; i++)
    {
        float f0 = calculateSpread(i, freq_val, spread_val);

        switch(i)
        {
            case Ui::WT_OSCS::WT_AUDIO_1:
            {
                vals.osc_mod_amount = osc_mod_depth_pot_1;
                if(state_->lfo_state_1)
                {
                    // This is roughly the ratio Brenso uses. Still exponential
                    // Might be nice for this to have a much larger range and
                    // go from very slow (~10 minutes) to low audio (~40 Hz)
                    f0 = f0 * 0.0054f;
                }
                break;
            }
            case Ui::WT_OSCS::WT_AUDIO_2:
            {
                vals.osc_mod_amount = 0.0f;
                if(state_->lfo_state_1)
                {
                    f0 = f0 * 0.0054f;
                }
                break;
            }
            case Ui::WT_OSCS::WT_AUDIO_3:
            {
                vals.osc_mod_amount = osc_mod_depth_pot_2;
                if(state_->lfo_state_2)
                {
                    f0 = f0 * 0.0054f;
                }
                break;
            }
            case Ui::WT_OSCS::WT_AUDIO_4:
            {
                vals.osc_mod_amount = 0.0f;
                if(state_->lfo_state_2)
                {
                    f0 = f0 * 0.0054f;
                }
                break;
            }
            default:
            {
                break;
            }
        }

        vals.frequency = daisysp::fclamp(
            f0 / hw_->AudioSampleRate(), kMinFrequency, kMaxFrequency);

        // x_spread_amt = pot + cv
        float x_spread_val = x_val + (x_spread_amt * spread_coeffs[i]);
        x_spread_val       = daisysp::fclamp(x_spread_val, 0.0f, 6.9999f);
        vals.x             = x_spread_val;

        float y_spread_val = y_val + (y_spread_amt * spread_coeffs[i]);
        y_spread_val       = daisysp::fclamp(y_spread_val, 0.0f, 6.9999f);
        vals.y             = y_spread_val;

        float z_spread_val = z_val + (z_spread_amt * spread_coeffs[i]);
        z_spread_val       = daisysp::fclamp(z_spread_val, 0.0f, 6.9999f);
        vals.z             = z_spread_val;

        synth_params_[i].Update(vals);
    }
}

Params::Values Ui::GetParams(size_t idx)
{
    return synth_params_[idx].Fetch();
}


void Ui::UpdateLEDs()

{
    // If waves failed to load, show breathing red
    if(!waves_loaded_)
    {
        // 2 second breathing cycle
        uint32_t elapsed = hw_->GetNow();
        float    phase   = (elapsed % 5000) / 5000.0f; // 0.0 to 1.0

        // Smooth sine wave breathing (0.0 to 1.0 and back)
        float brightness = (sinf(phase * 2.0f * M_PI) + 1.0f) / 2.0f;

        // Scale to LED range with minimum brightness
        uint8_t red
            = static_cast<uint8_t>(brightness * 245.0f + 10.0f); // 55-255 range

        for(auto& led : red_leds_)
        {
            hw_->led_driver.Set(led, red);
        }
        for(auto& led : green_leds_)
        {
            hw_->led_driver.Set(led, 0);
        }
        for(auto& led : blue_leds_)
        {
            hw_->led_driver.Set(led, 0);
        }
        hw_->UpdateLEDs();
        return;
    }

    constexpr uint8_t LED_COLOR_MID_R
        = crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, 0.5f);
    constexpr uint8_t LED_COLOR_MID_G
        = crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, 0.5f);
    constexpr uint8_t LED_COLOR_MID_B
        = crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, 0.5f);


    hw_->led_driver.Set(LED_WAVE_INTERPOLATE_TOGGLE,
                        state_->interpolate_waves ? 127 : 0);

    hw_->led_driver.Set(LED_TUNING_LOCK, lock_tuning_ ? 127 : 0);

    hw_->led_driver.Set(LED_LFO_1_TOGGLE, state_->lfo_state_1 ? 127 : 0);
    hw_->led_driver.Set(LED_LFO_2_TOGGLE, state_->lfo_state_2 ? 127 : 0);

    switch(state_->mod_state_1)
    {
        //rgb(255, 127, 80)
        case AppState::MOD_STATES::PHASE_MOD:
            hw_->led_driver.Set(LED_R_OSC_MODE_1, LED_COLOR_ONE_R);
            hw_->led_driver.Set(LED_G_OSC_MODE_1, LED_COLOR_ONE_G);
            hw_->led_driver.Set(LED_B_OSC_MODE_1, LED_COLOR_ONE_B);
            break;

        case AppState::MOD_STATES::WAVESHAPING:
            hw_->led_driver.Set(LED_R_OSC_MODE_1, LED_COLOR_TWO_R);
            hw_->led_driver.Set(LED_G_OSC_MODE_1, LED_COLOR_TWO_G);
            hw_->led_driver.Set(LED_B_OSC_MODE_1, LED_COLOR_TWO_B);
            break;

        case AppState::MOD_STATES::XOR:
            hw_->led_driver.Set(LED_R_OSC_MODE_1, LED_COLOR_MID_R);
            hw_->led_driver.Set(LED_G_OSC_MODE_1, LED_COLOR_MID_G);
            hw_->led_driver.Set(LED_B_OSC_MODE_1, LED_COLOR_MID_B);
            break;

        default: break;
    }

    switch(state_->mod_state_2)
    {
        //rgb(255, 127, 80)
        case AppState::MOD_STATES::PHASE_MOD:
            hw_->led_driver.Set(LED_R_OSC_MODE_2, LED_COLOR_ONE_R);
            hw_->led_driver.Set(LED_G_OSC_MODE_2, LED_COLOR_ONE_G);
            hw_->led_driver.Set(LED_B_OSC_MODE_2, LED_COLOR_ONE_B);
            break;

        case AppState::MOD_STATES::WAVESHAPING:
            hw_->led_driver.Set(LED_R_OSC_MODE_2, LED_COLOR_TWO_R);
            hw_->led_driver.Set(LED_G_OSC_MODE_2, LED_COLOR_TWO_G);
            hw_->led_driver.Set(LED_B_OSC_MODE_2, LED_COLOR_TWO_B);
            break;

        case AppState::MOD_STATES::XOR:
            hw_->led_driver.Set(LED_R_OSC_MODE_2, LED_COLOR_MID_R);
            hw_->led_driver.Set(LED_G_OSC_MODE_2, LED_COLOR_MID_G);
            hw_->led_driver.Set(LED_B_OSC_MODE_2, LED_COLOR_MID_B);
            break;

        default: break;
    }

    switch(state_->sync_mode_1)
    {
        case AppState::SYNC_MODES::HARD:
            hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_1, LED_COLOR_ONE_R);
            hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_1, LED_COLOR_ONE_G);
            hw_->led_driver.Set(LED_B_OSC_SYNC_TYPE_1, LED_COLOR_ONE_B);
            break;

        case AppState::SYNC_MODES::SOFT:
            hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_1, LED_COLOR_TWO_R);
            hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_1, LED_COLOR_TWO_G);
            hw_->led_driver.Set(LED_B_OSC_SYNC_TYPE_1, LED_COLOR_TWO_B);
            break;

        case AppState::SYNC_MODES::FLIP:
            hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_1, LED_COLOR_MID_R);
            hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_1, LED_COLOR_MID_G);
            hw_->led_driver.Set(LED_B_OSC_SYNC_TYPE_1, LED_COLOR_MID_B);
            break;

        default: break;
    }

    switch(state_->sync_mode_2)
    {
        case AppState::SYNC_MODES::HARD:
            hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_2, LED_COLOR_ONE_R);
            hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_2, LED_COLOR_ONE_G);
            hw_->led_driver.Set(LED_B_OSC_SYNC_TYPE_2, LED_COLOR_ONE_B);
            break;

        case AppState::SYNC_MODES::SOFT:
            hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_2, LED_COLOR_TWO_R);
            hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_2, LED_COLOR_TWO_G);
            hw_->led_driver.Set(LED_B_OSC_SYNC_TYPE_2, LED_COLOR_TWO_B);
            break;

        case AppState::SYNC_MODES::FLIP:
            hw_->led_driver.Set(LED_R_OSC_SYNC_TYPE_2, LED_COLOR_MID_R);
            hw_->led_driver.Set(LED_G_OSC_SYNC_TYPE_2, LED_COLOR_MID_G);
            hw_->led_driver.Set(LED_B_OSC_SYNC_TYPE_2, LED_COLOR_MID_B);
            break;

        default: break;
    }

    // X, Y, Z position LEDs
    float x_val = params[POT_X].Value()
                  + (cvs[CV_X_POSITION].Value() * params[POT_X_ATT].Value());
    x_val = daisysp::fclamp(x_val, 0.0f, 6.9999f);
    x_val /= 7.0f;

    float y_val = params[POT_Y].Value()
                  + (cvs[CV_Y_POSITION].Value() * params[POT_Y_ATT].Value());
    y_val = daisysp::fclamp(y_val, 0.0f, 6.9999f);
    y_val /= 7.0f;

    float z_val = params[POT_Z].Value()
                  + (cvs[CV_Z_POSITION].Value() * params[POT_Z_ATT].Value());
    z_val = daisysp::fclamp(z_val, 0.0f, 6.9999f);
    z_val /= 7.0f;

    hw_->led_driver.Set(LED_R_X_POS,
                        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, x_val));
    hw_->led_driver.Set(LED_G_X_POS,
                        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, x_val));
    hw_->led_driver.Set(LED_B_X_POS,
                        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, x_val));

    hw_->led_driver.Set(LED_R_Y_POS,
                        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, y_val));
    hw_->led_driver.Set(LED_G_Y_POS,
                        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, y_val));
    hw_->led_driver.Set(LED_B_Y_POS,
                        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, y_val));

    hw_->led_driver.Set(LED_R_Z_POS,
                        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, z_val));
    hw_->led_driver.Set(LED_G_Z_POS,
                        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, z_val));
    hw_->led_driver.Set(LED_B_Z_POS,
                        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, z_val));

    // X, Y, Z spread LEDs
    float x_spread_val
        = params[POT_X_SPREAD].Value() + cvs[CV_X_SPREAD].Value();
    x_spread_val = daisysp::fclamp(x_spread_val, -1.0f, 1.0f);
    x_spread_val = (x_spread_val + 1) / 2.0f;

    float y_spread_val
        = params[POT_Y_SPREAD].Value() + cvs[CV_Y_SPREAD].Value();
    y_spread_val = daisysp::fclamp(y_spread_val, -1.0f, 1.0f);
    y_spread_val = (y_spread_val + 1) / 2.0f;

    float z_spread_val
        = params[POT_Z_SPREAD].Value() + cvs[CV_Z_SPREAD].Value();
    z_spread_val = daisysp::fclamp(z_spread_val, -1.0f, 1.0f);
    z_spread_val = (z_spread_val + 1) / 2.0f;

    hw_->led_driver.Set(
        LED_R_X_SPREAD,
        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, x_spread_val));
    hw_->led_driver.Set(
        LED_G_X_SPREAD,
        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, x_spread_val));
    hw_->led_driver.Set(
        LED_B_X_SPREAD,
        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, x_spread_val));

    hw_->led_driver.Set(
        LED_R_Y_SPREAD,
        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, y_spread_val));
    hw_->led_driver.Set(
        LED_G_Y_SPREAD,
        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, y_spread_val));
    hw_->led_driver.Set(
        LED_B_Y_SPREAD,
        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, y_spread_val));

    hw_->led_driver.Set(
        LED_R_Z_SPREAD,
        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, z_spread_val));
    hw_->led_driver.Set(
        LED_G_Z_SPREAD,
        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, z_spread_val));
    hw_->led_driver.Set(
        LED_B_Z_SPREAD,
        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, z_spread_val));

    // Frequency spread LED
    float spread_val = params[POT_FREQ_SPREAD].Value()
                       + (cvs[CV_TUNING_SPREAD].Value()
                          * params[POT_FREQ_SPREAD_ATT].Value());
    spread_val = daisysp::fclamp(spread_val, -1.0f, 1.0f);
    spread_val = (spread_val + 1) / 2.0f;

    hw_->led_driver.Set(
        LED_R_FREQ_SPREAD,
        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, spread_val));
    hw_->led_driver.Set(
        LED_G_FREQ_SPREAD,
        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, spread_val));
    hw_->led_driver.Set(
        LED_B_FREQ_SPREAD,
        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, spread_val));

    // Frequency LED
    float freq_val = freq_pots_
                     + vcal_.ProcessInput(hw_->cv[hw_->ONBOARD_CV_0].Value())
                     + (cvs[CV_FM].Value() * params[POT_FM_ATT].Value());

    freq_val /= 128;
    freq_val = daisysp::fclamp(freq_val, 0.0f, 1.0f);
    hw_->led_driver.Set(LED_R_FM_AMT,
                        crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, freq_val));
    hw_->led_driver.Set(LED_G_FM_AMT,
                        crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, freq_val));
    hw_->led_driver.Set(LED_B_FM_AMT,
                        crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, freq_val));

    hw_->UpdateLEDs();
}

void Ui::SetLEDsRed()
{
    SetLEDsByChannel(red_leds_, 255);
}

void Ui::SetLEDsGreen()
{
    SetLEDsByChannel(green_leds_, 255);
}

void Ui::SetLEDsBlue()
{
    SetLEDsByChannel(blue_leds_, 255);
}

void Ui::SetLEDsPurple()
{
    SetLEDsByChannel(red_leds_, 255);
    SetLEDsByChannel(blue_leds_, 255);
}

void Ui::SetLEDsByChannel(const std::array<fourseas::Ui::LEDs, 12> chan,
                          uint8_t                                  val)
{
    for(auto& led : chan)
    {
        hw_->led_driver.Set(led, val);
    }
    hw_->UpdateLEDs();
}

void Ui::SetLEDsByValue(uint8_t val)
{
    for(size_t i = 0; i < LED_LAST; i++)
    {
        hw_->led_driver.Set(i, val);
    }
    hw_->UpdateLEDs();
}

void Ui::SetLEDsOff()
{
    SetLEDsByValue(0);
}

void Ui::SetLEDsOn()
{
    SetLEDsByValue(255);
}

void Ui::StartupLEDSequence()
{
    constexpr uint32_t fade_duration   = 1500; // 1.5 seconds
    constexpr uint32_t update_interval = 20;   // 50fps

    uint32_t start_time  = hw_->GetNow();
    uint32_t last_update = start_time;

    while((hw_->GetNow() - start_time) < fade_duration)
    {
        uint32_t current_time = hw_->GetNow();
        if(current_time - last_update >= update_interval)
        {
            last_update = current_time;

            // Calculate fade progress (0.0 to 1.0)
            float progress = (float)(current_time - start_time) / fade_duration;

            // Crossfade from white (255,255,255) to blue (0,145,255)
            uint8_t r = crossfade(255, LED_COLOR_ONE_R, progress);
            uint8_t g = crossfade(255, LED_COLOR_ONE_G, progress);
            uint8_t b = crossfade(255, LED_COLOR_ONE_B, progress);

            // Set all RGB LEDs using the correct arrays
            for(const auto& led : red_leds_)
            {
                hw_->led_driver.Set(led, r);
            }
            for(const auto& led : green_leds_)
            {
                hw_->led_driver.Set(led, g);
            }
            for(const auto& led : blue_leds_)
            {
                hw_->led_driver.Set(led, b);
            }

            hw_->UpdateLEDs();
        }
        hw_->DelayMs(5);
    }
}

void Ui::FadeBankLED(size_t bank_index)
{
    constexpr uint32_t fade_duration   = 150;
    constexpr uint32_t update_interval = 10;

    uint32_t start_time  = hw_->GetNow();
    uint32_t last_update = start_time;


    if(bank_index >= sizeof(bank_leds_) / sizeof(bank_leds_[0]))
    {
        return; // Invalid bank index
    }

    while((hw_->GetNow() - start_time) < fade_duration)
    {
        uint32_t current_time = hw_->GetNow();
        if(current_time - last_update >= update_interval)
        {
            last_update = current_time;

            // Calculate fade progress (0.0 to 1.0)
            float progress = (float)(current_time - start_time) / fade_duration;

            // Crossfade from color one to color two
            uint8_t r = crossfade(LED_COLOR_ONE_R, LED_COLOR_TWO_R, progress);
            uint8_t g = crossfade(LED_COLOR_ONE_G, LED_COLOR_TWO_G, progress);
            uint8_t b = crossfade(LED_COLOR_ONE_B, LED_COLOR_TWO_B, progress);

            // Set the specific bank's LEDs
            hw_->led_driver.Set(bank_leds_[bank_index][0], r);
            hw_->led_driver.Set(bank_leds_[bank_index][1], g);
            hw_->led_driver.Set(bank_leds_[bank_index][2], b);

            hw_->UpdateLEDs();
        }
        hw_->DelayMs(2);
    }
}

void Ui::FadeToColor(uint8_t  target_r,
                     uint8_t  target_g,
                     uint8_t  target_b,
                     uint32_t duration_ms)
{
    constexpr uint32_t update_interval = 20; // 50fps

    uint32_t start_time  = hw_->GetNow();
    uint32_t last_update = start_time;

    // Fades from black in this case
    uint8_t start_r = 0, start_g = 0, start_b = 0;

    while((hw_->GetNow() - start_time) < duration_ms)
    {
        uint32_t current_time = hw_->GetNow();
        if(current_time - last_update >= update_interval)
        {
            last_update = current_time;

            // Calculate fade progress (0.0 to 1.0)
            float progress = (float)(current_time - start_time) / duration_ms;

            // Crossfade to target color
            uint8_t r = crossfade(start_r, target_r, progress);
            uint8_t g = crossfade(start_g, target_g, progress);
            uint8_t b = crossfade(start_b, target_b, progress);

            // Set all RGB LEDs using the existing arrays
            for(const auto& led : red_leds_)
                hw_->led_driver.Set(led, r);
            for(const auto& led : green_leds_)
                hw_->led_driver.Set(led, g);
            for(const auto& led : blue_leds_)
                hw_->led_driver.Set(led, b);

            hw_->UpdateLEDs();
        }
        hw_->DelayMs(5);
    }
}

uint8_t Ui::GetBankNum()
{
    return bank_num_;
}

void Ui::SetBanksMax(uint8_t max_banks)
{
    max_banks_ = max_banks;
}

void Ui::SetWavesLoaded(bool loaded)
{
    waves_loaded_ = loaded;
}

bool Ui::isButtonPressed(uint8_t idx)
{
    if(idx > SW_LAST)
    {
        return false;
    }
    return hw_->buttons[idx].isPressed();
}

bool Ui::isButtonReleased(uint8_t idx)
{
    if(idx > SW_LAST)
    {
        return false;
    }
    return hw_->buttons[idx].isReleased();
}

bool Ui::isButtonUp(uint8_t idx)
{
    if(idx > SW_LAST)
    {
        return false;
    }
    return hw_->buttons[idx].isUp();
}

bool Ui::isButtonDown(uint8_t idx)
{
    if(idx > SW_LAST)
    {
        return false;
    }
    return hw_->buttons[idx].isDown();
}

float ipiConverge(const float factors[], size_t idx, float freq, float spread)
{
    /*
        ratio_interpolated = (target_ratio / initial_ratio) ^ spread

        Calculation: ratio_interpolated = (2 / 1) ^ 0.5 = 2 ^ 0.5 ≈ 1.414 (square root of 2)
        Interpolated Frequency: f_interpolated = f0 * ratio_interpolated
    */
    float ratio = factors[idx];
    if(ratio == 0.0f)
    {
        return freq;
    }

    if(spread < 0.0f)
    {
        ratio  = 1.0f / ratio; // Invert the ratio for negative spread
        spread = -spread;      // Use the absolute value of spread
    }

    float ratio_interpolated = powf(ratio, spread);
    return freq * ratio_interpolated;
}

float Ui::calculateSpread(size_t idx, float freq, float spread)
{
    switch(spread_type_)
    {
        case AppState::SPREAD_TYPES::SPREAD_ONE:
        {
            // Just intonation
            const float factors[4] = {1.0f, 1.25f, 1.5f, 2.0f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_TWO:
        {
            // normal (sub)harmonics
            const float factors[4] = {1.0f, 2.0f, 3.0f, 4.0f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_THREE:
        {
            // Pythagorean Tuning (Fifths)
            const float factors[4] = {1.0f, 1.5f, 2.25f, 3.375f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_FOUR:
        {
            // Subharmonic Superparticular Ratios
            const float factors[4] = {1.0f, 0.8f, 0.75f, 0.66666666667f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_FIVE:
        {
            // Fibonacci
            const float factors[4] = {1.0f, 1.5f, 1.6f, 1.66666666667f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_SIX:
        {
            // Spectral √2
            const float factors[4] = {1, 1.414f, 2, 2.828f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_SEVEN:
        {
            // Golden ratio
            const float factors[4] = {1.0f, 1.618f, 2.618f, 4.236f};
            return ipiConverge(factors, idx, freq, spread);
        }
        case AppState::SPREAD_TYPES::SPREAD_EIGHT:
        {
            // New: "Bohlen-Pierce" Mode
            const float factors[4]
                = {1.0f, 1.66666666667f, 2.33333333337f, 3.0f};
            return ipiConverge(factors, idx, freq, spread);
        }
        default: break;
    }
    return freq;
}

} // namespace fourseas
