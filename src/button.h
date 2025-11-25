#pragma once

#include "daisy.h"

namespace fourseas
{

constexpr uint16_t PRESSED_MASK   = 0xf83f;
constexpr uint16_t PRESSED_EQUALS = 0x3f;

constexpr uint16_t RELEASED_MASK   = 0xfc1f;
constexpr uint16_t RELEASED_EQUALS = 0xfc00;


class Button
{
  public:
    uint16_t* GetState();
    bool      isPressed();
    bool      isReleased();
    bool      isUp();
    bool      isDown();
    uint32_t  TimeHeldMs();
    void      UpdateTimestamp();


  private:
    uint16_t state_        = 0;
    uint32_t pressed_time_ = 0;
    bool     was_down_     = false;
};


inline uint16_t* Button::GetState()
{
    return &state_;
}

inline bool Button::isPressed()
{
    if((state_ & PRESSED_MASK) == PRESSED_EQUALS)
    {
        pressed_time_ = daisy::System::GetNow();
        state_        = 0xffff;
        return true;
    }
    return false;
}

inline bool Button::isReleased()
{
    if((state_ & RELEASED_MASK) == RELEASED_EQUALS)
    {
        state_ = 0;
        return true;
    }
    return false;
}

inline bool Button::isUp()
{
    return state_ == 0;
}

inline bool Button::isDown()
{
    return state_ == 0xffff;
}

inline uint32_t Button::TimeHeldMs()
{
    return isDown() ? daisy::System::GetNow() - pressed_time_ : 0;
}

inline void Button::UpdateTimestamp()
{
    bool is_down_now = isDown();

    if(!was_down_ && is_down_now)
    {
        pressed_time_ = daisy::System::GetNow();
    }

    was_down_ = is_down_now;
}


} // namespace fourseas
