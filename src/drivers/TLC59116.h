#pragma once

#include "daisy.h"

class TLC59116
{
  public:
    enum RegisterAddresses
    {
        MODE1      = 0x00,
        MODE2      = 0x01,
        PWM0       = 0x02,
        PWM1       = 0x03,
        PWM2       = 0x04,
        PWM3       = 0x05,
        PWM4       = 0x06,
        PWM5       = 0x07,
        PWM6       = 0x08,
        PWM7       = 0x09,
        PWM8       = 0x0A,
        PWM9       = 0x0B,
        PWM10      = 0x0C,
        PWM11      = 0x0D,
        PWM12      = 0x0E,
        PWM13      = 0x0F,
        PWM14      = 0x10,
        PWM15      = 0x11,
        GRPPWM     = 0x12,
        GRPFREQ    = 0x13,
        LEDOUT0    = 0x14,
        LEDOUT1    = 0x15,
        LEDOUT2    = 0x16,
        LEDOUT3    = 0x17,
        SUBADR1    = 0x18,
        SUBADR2    = 0x19,
        SUBADR3    = 0x1A,
        ALLCALLADR = 0x1B,
        IREF       = 0x1C,
        EFLAG1     = 0x1D,
        EFLAG2     = 0x1E
    };

    TLC59116() {}
    ~TLC59116() {}

    void Init();
    void Set(uint8_t idx, uint8_t level);
    void Transmit();

  private:
    uint8_t          i2c_addr_[3];
    daisy::I2CHandle i2c_handle_;
    uint8_t          default_allcall_ = 0b1101000; // 0x68
    uint8_t          current_device_idx_;

    void static endCallback_(void *context, daisy::I2CHandle::Result result)
    {
        TLC59116 *instance = static_cast<TLC59116 *>(context);
        if(instance->current_device_idx_ >= 3)
        {
            instance->current_device_idx_ = 0;
            return;
        }
        instance->current_device_idx_++;
        instance->Transmit();
    }
};