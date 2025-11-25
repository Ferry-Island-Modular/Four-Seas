// Copyright 2023 Justin Goney.
//
// Author: Justin Goney (justin.goney@gmail.com)
// Modified from code originally written by Emilie Gillet (emilie.o.gillet@gmail.com)
// https://github.com/pichenettes/eurorack
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------

#pragma once

#include <algorithm>

#include "stmlib/dsp/dsp.h"

#include "src/ui.h"
#include "src/constants.h"

namespace fourseas
{
template <typename T>
inline float
InterpolateWave(const T* table, int32_t index_integral, float index_fractional)
{
    float a = table[index_integral];
    float b = table[index_integral + 1];
    float t = index_fractional;
    return a + (b - a) * t;
}


inline float Clamp(float x, float amount)
{
    x = (x - 0.5f) * amount;
    x = fminf(0.5f, fmaxf(-0.5f, x));
    return x + 0.5f;
}

template <size_t wavetable_size,
          bool   uses_sync       = false,
          bool   uses_modulation = false>
class WavetableOscillator
{
  public:
    WavetableOscillator() = default;
    ~WavetableOscillator() {}

    void Init(float** wavetable)
    {
        phase_ = 0.0f;

        wavetable_ = wavetable;
        all_waves_ = wavetable;
        prev_sync_ = false;
    }

    float
    ReadWave(int x, int y, int z, int phase_integral, float phase_fractional)
    {
        const float* wave = wavetable_[x + y * 8 + z * kNumWavesPerBank];
        return InterpolateWave(wave, phase_integral, phase_fractional);
    }

    void SetBank(size_t bank_idx) { wavetable_ = &all_waves_[bank_idx * 512]; }

    void Render(const OscillatorParams& params, float* out)
    {
        const float f0          = params.values.frequency;
        float       x           = params.values.x;
        float       y           = params.values.y;
        float       z           = params.values.z;
        float       mod_amount  = params.values.osc_mod_amount;
        bool        interpolate = params.interpolate;
        uint8_t     mod_state   = params.mod_state;
        float       mod_input   = params.mod_input;

        uint8_t sync_state = params.sync_state;
        bool    sync_input = params.sync_input;


        // flip this so that true actually equals true
        interpolate = !interpolate;

        float phase = phase_;

        phase += (is_flipped_ ? f0 : -f0);

        if constexpr(uses_modulation)
        {
            if(mod_state == AppState::MOD_STATES::PHASE_MOD)
            {
                static constexpr float pmFactor = kMaxFrequency / 2.0f;
                mod_amount = fourseas::DeadZone(mod_amount, 0.01f) * pmFactor;
                phase += mod_input * mod_amount;
            }
            else if(mod_state == AppState::MOD_STATES::WAVESHAPING)
            {
                // Pre-clamp inputs once
                mod_input  = daisysp::fclamp(mod_input, -1.0f, 1.0f);
                mod_amount = daisysp::fclamp(mod_amount, 0.0f, 1.0f);

                if(mod_amount < 0.1f)
                {
                    *out = 0.0f;
                    return;
                }

                // Waveshaping calculation
                float x  = mod_input * mod_amount;
                float x2 = x * x;

                static constexpr float one_third = 1.0f / 3.0f;
                float                  saturated = x * (1.0f - x2 * one_third);

                phase = (saturated + 1.0f) * 0.5f;
            }
        }

        if(mod_state != AppState::MOD_STATES::WAVESHAPING)
        {
            phase = phase - floorf(phase);
        }

        if constexpr(uses_sync)
        {
            if(prev_sync_ == false && sync_input == true)
            {
                // TODO: experiment with PLL here for sync
                // (though soft sync seems to work okay in cases)
                switch(sync_state)
                {
                    case AppState::SYNC_MODES::HARD:
                    {
                        phase = 0.0f;
                        break;
                    }
                    // Orange
                    case AppState::SYNC_MODES::SOFT:
                    {
                        if(phase <= 0.25f)
                        {
                            phase = 0.0f;
                        }
                        break;
                    }
                    case AppState::SYNC_MODES::FLIP:
                    {
                        is_flipped_ = !is_flipped_;
                        break;
                    }
                    default: break;
                }
            }
            prev_sync_ = sync_input;
        }

        // int32_t x_integral = static_cast<int32_t>(x);
        // float x_fractional = x - static_cast<float>(x_integral);
        MAKE_INTEGRAL_FRACTIONAL(x);
        MAKE_INTEGRAL_FRACTIONAL(y);
        MAKE_INTEGRAL_FRACTIONAL(z);

        // Interpolation stuff
        x_fractional
            += interpolate * (Clamp(x_fractional, 16.0f) - x_fractional);
        y_fractional
            += interpolate * (Clamp(y_fractional, 16.0f) - y_fractional);
        z_fractional
            += interpolate * (Clamp(z_fractional, 16.0f) - z_fractional);

        // TODO: ensure that this "off by one" thing is correct and glitch-free
        const float p = phase * float(wavetable_size) - 1;
        MAKE_INTEGRAL_FRACTIONAL(p);

        int x0 = x_integral;
        int x1 = x_integral + 1;
        int y0 = y_integral;
        int y1 = y_integral + 1;
        int z0 = z_integral;
        int z1 = z_integral + 1;

        float x0y0z0 = ReadWave(x0, y0, z0, p_integral, p_fractional);
        float x1y0z0 = ReadWave(x1, y0, z0, p_integral, p_fractional);
        float xy0z0  = x0y0z0 + (x1y0z0 - x0y0z0) * x_fractional;

        float x0y1z0 = ReadWave(x0, y1, z0, p_integral, p_fractional);
        float x1y1z0 = ReadWave(x1, y1, z0, p_integral, p_fractional);
        float xy1z0  = x0y1z0 + (x1y1z0 - x0y1z0) * x_fractional;

        float xyz0 = xy0z0 + (xy1z0 - xy0z0) * y_fractional;

        float x0y0z1 = ReadWave(x0, y0, z1, p_integral, p_fractional);
        float x1y0z1 = ReadWave(x1, y0, z1, p_integral, p_fractional);
        float xy0z1  = x0y0z1 + (x1y0z1 - x0y0z1) * x_fractional;

        float x0y1z1 = ReadWave(x0, y1, z1, p_integral, p_fractional);
        float x1y1z1 = ReadWave(x1, y1, z1, p_integral, p_fractional);
        float xy1z1  = x0y1z1 + (x1y1z1 - x0y1z1) * x_fractional;

        float xyz1 = xy0z1 + (xy1z1 - xy0z1) * y_fractional;

        float mix = xyz0 + (xyz1 - xyz0) * z_fractional;

        if constexpr(uses_modulation)
        {
            if(mod_state == AppState::MOD_STATES::XOR)
            {
                mod_amount = daisysp::fclamp(mod_amount, 0.1f, 1.0f);

                // Convert to 12-bit range with more aggressive scaling
                int16_t a = static_cast<int16_t>((mix + 1.0f) * 2047.0f);
                int16_t b = static_cast<int16_t>((mod_input + 1.0f) * mod_amount
                                                 * 2047.0f);

                // More aggressive bit mangling
                int16_t result = ((a & b) << 1) ^ (a | b);

                // Add some normalization to prevent too much attenuation
                result = (result * 3) >> 1;

                // Back to float
                mix = (static_cast<float>(result) / 2047.0f) - 1.0f;

                // Ensure we stay in range
                mix = daisysp::fclamp(mix, -1.0f, 1.0f);
            }
        }

        *out = mix;

        phase_ = phase;
    }

  private:
    // Oscillator state.
    float phase_;

    bool prev_sync_;
    bool is_flipped_;

    float** wavetable_;
    float** all_waves_;

    uint8_t bank_;
};

} // namespace fourseas
