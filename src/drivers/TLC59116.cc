#include <algorithm>
#include <array>
#include "daisy.h"

#include "daisy_seed.h"
#include "TLC59116.h"

constexpr daisy::Pin PIN_I2C1_SCL = daisy::seed::D11;
constexpr daisy::Pin PIN_I2C1_SDA = daisy::seed::D12;

constexpr size_t BUFFER_SIZE = 17;

static uint8_t DMA_BUFFER_MEM_SECTION tlc59116_i2c_tx_buffer[3][BUFFER_SIZE];


void TLC59116::Init()
{
    i2c_addr_[0]        = 0b1100000; // 0x60?
    i2c_addr_[1]        = 0b1100001; // 0x61?
    i2c_addr_[2]        = 0b1100010; // 0x62?
    current_device_idx_ = 0;

    // Zero-out buffer
    for(auto &buffer : tlc59116_i2c_tx_buffer)
    {
        std::fill(&buffer[0], &buffer[BUFFER_SIZE - 1], 0);
    }

    daisy::I2CHandle::Config i2c_config;

    i2c_config.periph = daisy::I2CHandle::Config::Peripheral::I2C_1;

    i2c_config.speed          = daisy::I2CHandle::Config::Speed::I2C_1MHZ;
    i2c_config.mode           = daisy::I2CHandle::Config::Mode::I2C_MASTER;
    i2c_config.pin_config.scl = PIN_I2C1_SCL;
    i2c_config.pin_config.sda = PIN_I2C1_SDA;

    i2c_handle_.Init(i2c_config);

    static constexpr uint16_t size    = 2;
    static constexpr uint32_t timeout = 10000;

    // reset MODE1 register
    uint8_t reg  = TLC59116::MODE1;
    uint8_t data = 0x01; // leave defaults but turn oscillator on

    uint8_t payload[2] = {reg, data};

    i2c_handle_.TransmitBlocking(default_allcall_, payload, size, timeout);

    payload[0] = TLC59116::LEDOUT0;
    payload[1] = 0xFF;
    i2c_handle_.TransmitBlocking(default_allcall_, payload, size, timeout);

    payload[0] = TLC59116::LEDOUT1;
    payload[1] = 0xFF;
    i2c_handle_.TransmitBlocking(default_allcall_, payload, size, timeout);

    payload[0] = TLC59116::LEDOUT2;
    payload[1] = 0xFF;
    i2c_handle_.TransmitBlocking(default_allcall_, payload, size, timeout);

    payload[0] = TLC59116::LEDOUT3;
    payload[1] = 0xFF;
    i2c_handle_.TransmitBlocking(default_allcall_, payload, size, timeout);

    // Apply global brightness control (roughly 30% of max)
    // power target, under 300mA
    payload[0] = TLC59116::GRPPWM;
    payload[1] = 0x66; // dec 102
    //payload[1] = 0xFF; // full power


    i2c_handle_.TransmitBlocking(default_allcall_, payload, size, timeout);
}

void TLC59116::Set(uint8_t idx, uint8_t level)
{
    uint8_t buff_idx = idx / 16;
    idx -= (buff_idx * 16);
    tlc59116_i2c_tx_buffer[buff_idx][idx + 1] = level;
}

void TLC59116::Transmit()
{
    static constexpr uint8_t reg = TLC59116::PWM0;

    tlc59116_i2c_tx_buffer[current_device_idx_][0] = (0b101 << 5) | reg;
    i2c_handle_.TransmitDma(i2c_addr_[current_device_idx_],
                            tlc59116_i2c_tx_buffer[current_device_idx_],
                            BUFFER_SIZE,
                            endCallback_,
                            this);
}
