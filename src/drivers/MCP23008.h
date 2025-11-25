#pragma once

#include "daisy.h"

class MCP23008
{
  public:
    enum RegisterAddresses
    {
        IODIR   = 0x00,
        IPOL    = 0x01,
        GPINTEN = 0x02,
        DEFVAL  = 0x03,
        INTCON  = 0x04,
        IOCON   = 0x05,
        GPPU    = 0x06,
        INTF    = 0x07,
        INTCAP  = 0x08, //(Read-only)
        GPIO    = 0x09,
        OLAT    = 0x0A,
    };

    MCP23008() {}
    ~MCP23008() {}

    void    Init(uint8_t addr);
    uint8_t Fetch();

  private:
    uint8_t          i2c_addr_;
    daisy::I2CHandle i2c_handle_;
};