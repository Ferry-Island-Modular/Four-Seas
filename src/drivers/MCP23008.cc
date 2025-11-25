#include "daisy.h"

#include "daisy_seed.h"
#include "MCP23008.h"

constexpr daisy::Pin PIN_I2C1_SCL = daisy::seed::D11; // 12
constexpr daisy::Pin PIN_I2C1_SDA = daisy::seed::D12; // 13

void MCP23008::Init(uint8_t addr)
{
    i2c_addr_ = addr;

    daisy::I2CHandle::Config i2c_config;

    i2c_config.periph = daisy::I2CHandle::Config::Peripheral::I2C_1;

    i2c_config.speed          = daisy::I2CHandle::Config::Speed::I2C_1MHZ;
    i2c_config.mode           = daisy::I2CHandle::Config::Mode::I2C_MASTER;
    i2c_config.pin_config.scl = PIN_I2C1_SCL;
    i2c_config.pin_config.sda = PIN_I2C1_SDA;

    i2c_handle_.Init(i2c_config);

    // Init device
    static constexpr uint16_t size    = 2;
    static constexpr uint32_t timeout = 10000;

    uint8_t reg  = MCP23008::GPPU;
    uint8_t data = 0xFF;

    uint8_t payload[2] = {reg, data};
    i2c_handle_.TransmitBlocking(i2c_addr_, payload, size, timeout);

    reg        = MCP23008::IPOL;
    payload[0] = reg;
    payload[1] = 0xFF;
    i2c_handle_.TransmitBlocking(i2c_addr_, payload, size, timeout);
}

uint8_t MCP23008::Fetch()
{
    static constexpr uint32_t timeout = 10000;

    // reset MODE1 register
    static constexpr uint8_t reg     = MCP23008::GPIO; // MODE1 register
    uint8_t                  data[1] = {0x00};

    i2c_handle_.ReadDataAtAddress(i2c_addr_ << 1, reg, 1, data, 1, timeout);

    return data[0];
}