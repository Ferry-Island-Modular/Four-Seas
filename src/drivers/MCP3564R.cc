#include <algorithm>

#include "daisy.h"

#include "daisy_seed.h"
#include "MCP3564R.h"


// Device constants: registers, defaults, etc.
const uint8_t kDeviceAddress  = (01 << 6);
const uint8_t kRegAdcData     = 0;
const uint8_t kRegIRQ         = 0x5;
const uint8_t kRegMultiplexer = 0x6;
const uint8_t kRegScan        = 0x7;

const uint8_t kRegConfig0Default = 0b11000000;
const uint8_t kRegConfig1Default = 0b00001100;
const uint8_t kRegConfig2Default = 0b10001011;
const uint8_t kRegConfig3Default = 0b0;

const uint8_t kRegIRQDefault = 0b01110011;
const uint8_t kRegMUXDefault = 0b1;


// SPI pins
constexpr daisy::Pin ADC_PIN_SPI1_MOSI = daisy::seed::D10; // Seed2 B4
constexpr daisy::Pin ADC_PIN_SPI1_MISO = daisy::seed::D9;  // Seed2 B1
constexpr daisy::Pin ADC_PIN_SPI1_SCK  = daisy::seed::D8;  // Seed2 B2
constexpr daisy::Pin ADC_PIN_SPI1_CS   = daisy::seed::D7;  // Seed2 B6

constexpr uint8_t NUM_ADC_CHANNELS = 8;

uint32_t channel_values[8];

constexpr size_t BUFFER_SIZE = 33;

/** outside of class static buffer(s) for DMA access */
static uint8_t DMA_BUFFER_MEM_SECTION spi_tx_buffer[BUFFER_SIZE];
static uint8_t DMA_BUFFER_MEM_SECTION spi_rx_buffer[BUFFER_SIZE];

inline void constructTxData(uint8_t*                    buffer,
                            AdcMCP3564R::ConfigRegister confReg,
                            AdcMCP3564R::CommandType    cmd,
                            uint8_t                     data = 0)
{
    buffer[0] = static_cast<uint8_t>(kDeviceAddress | (confReg << 2) | cmd);
    buffer[1] = data;
}

AdcMCP3564R::Result AdcMCP3564R::Init()
{
    daisy::SpiHandle::Config spi_config = daisy::SpiHandle::Config();

    // Init buffers cleared out
    std::fill(&spi_tx_buffer[0], &spi_tx_buffer[BUFFER_SIZE - 1], 0);
    std::fill(&spi_rx_buffer[0], &spi_rx_buffer[BUFFER_SIZE - 1], 0);


    spi_tx_buffer[0] = kDeviceAddress | (kRegAdcData << 2)
                       | AdcMCP3564R::CommandType::STATIC_READ;

    val_ = channel_values;

    spi_config.periph    = daisy::SpiHandle::Config::Peripheral::SPI_1;
    spi_config.mode      = daisy::SpiHandle::Config::Mode::MASTER;
    spi_config.direction = daisy::SpiHandle::Config::Direction::TWO_LINES;
    spi_config.nss       = daisy::SpiHandle::Config::NSS::HARD_OUTPUT;

    spi_config.pin_config.sclk = ADC_PIN_SPI1_SCK;
    spi_config.pin_config.miso = ADC_PIN_SPI1_MISO;
    spi_config.pin_config.mosi = ADC_PIN_SPI1_MOSI;
    spi_config.pin_config.nss  = ADC_PIN_SPI1_CS;

    if(spi_handle_.Init(spi_config) != daisy::SpiHandle::Result::OK)
    {
        return AdcMCP3564R::Result::ERR;
    }
    return AdcMCP3564R::Result::OK;
}

AdcMCP3564R::Result AdcMCP3564R::ResetDefaults()
{
    // 0b01 1110 00
    uint8_t tx[1] = {
        kDeviceAddress | (0b1110 << 2) | AdcMCP3564R::CommandType::FAST_CMD,
    };

    uint8_t rx[1];

    uint16_t size    = 1;
    uint32_t timeout = 100;

    if(spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout)
       != daisy::SpiHandle::Result::OK)
    {
        return AdcMCP3564R::Result::ERR;
    }
    return AdcMCP3564R::Result::OK;
}


uint8_t AdcMCP3564R::GetConfig(AdcMCP3564R::ConfigRegister reg)
{
    constexpr uint16_t size = 2;


    uint8_t tx[size];
    constructTxData(tx, reg, AdcMCP3564R::CommandType::STATIC_READ);

    uint8_t rx[size];

    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);

    return rx[1];
}

void AdcMCP3564R::UseInternalVRef(bool ref)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG0;


    uint8_t conf_val = GetConfig(confReg);

    uint8_t data;
    if(ref)
    {
        data = conf_val | (1 << 7);
    }
    else
    {
        data = conf_val & ~(1 << 7);
    }

    uint8_t tx[2];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetClock(AdcMCP3564R::ClkSel clk)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG0;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t bits     = clk << 4;
    uint8_t mask     = 0b00110000;

    uint8_t data = (conf_val & ~mask) | (bits & mask);

    uint16_t size = 2;

    uint8_t tx[size];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[size];

    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetAMCLKPrescaler(AdcMCP3564R::AMCLKSel clkps)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG1;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t bits     = clkps << 6;
    uint8_t mask     = 0b11000000;

    uint8_t data = (conf_val & ~mask) | (bits & mask);

    uint8_t tx[2];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetBoost(AdcMCP3564R::BoostSel boost)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG2;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t bits     = boost << 6;
    uint8_t mask     = 0b11000000;

    uint8_t data = (conf_val & ~mask) | (bits & mask);

    uint8_t tx[2];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}


void AdcMCP3564R::SetOversamplingRatio(AdcMCP3564R::OSRSel sr)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG1;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t bits     = sr << 2;
    uint8_t mask     = 0b00111100;

    uint8_t data = (conf_val & ~mask) | (bits & mask);


    uint8_t tx[2];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetConversionMode(AdcMCP3564R::ConversionMode mode)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG3;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t bits     = mode << 6;
    uint8_t mask     = 0b11 << 6;

    uint8_t data = (conf_val & ~mask) | (bits & mask);

    uint8_t tx[2];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetADCMode(AdcMCP3564R::ADCMode mode)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG0;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t mask     = 0b11;

    uint8_t data = (conf_val & ~mask) | (mode & mask);

    uint8_t tx[2];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetMuxReg(AdcMCP3564R::MuxMode msb, AdcMCP3564R::MuxMode lsb)
{
    uint8_t data = (msb << 4) | lsb;

    uint8_t tx[2] = {kDeviceAddress | (kRegMultiplexer << 2)
                         | AdcMCP3564R::CommandType::INCR_WRITE,
                     data};

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetIRQPin()
{
    // Bits [8:4] are read only
    uint8_t data = 0b00000011;

    uint8_t tx[2] = {kDeviceAddress | (kRegIRQ << 2)
                         | AdcMCP3564R::CommandType::INCR_WRITE,
                     data};

    uint8_t rx[2];

    uint16_t size    = 2;
    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetDataFormat(AdcMCP3564R::DataFormats df)
{
    AdcMCP3564R::ConfigRegister confReg = AdcMCP3564R::ConfigRegister::CONFIG3;

    uint8_t conf_val = GetConfig(confReg);
    uint8_t bits     = df << 4;
    uint8_t mask     = 0b11 << 4;

    uint8_t data = (conf_val & ~mask) | (bits & mask);

    constexpr uint16_t size = 2;

    uint8_t tx[size];
    constructTxData(tx, confReg, AdcMCP3564R::CommandType::INCR_WRITE, data);

    uint8_t rx[size];

    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

void AdcMCP3564R::SetScanRegister(AdcMCP3564R::ScanDelayTimes scan_delay,
                                  uint16_t                    channels)
{
    constexpr uint16_t size = 4;
    uint8_t            tx[size]
        = {static_cast<uint8_t>(kDeviceAddress | (kRegScan << 2)
                                | AdcMCP3564R::CommandType::INCR_WRITE),
           static_cast<uint8_t>(scan_delay << 6),
           static_cast<uint8_t>(channels >> 8),
           static_cast<uint8_t>(channels & 0xff)};

    uint8_t rx[size];

    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
}

uint32_t AdcMCP3564R::GetScanRegister()
{
    constexpr uint16_t size     = 4;
    uint8_t            tx[size] = {kDeviceAddress | (kRegScan << 2)
                                   | AdcMCP3564R::CommandType::STATIC_READ};

    uint8_t rx[size];

    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);
    return rx[0] << 16 | rx[1] << 8 | rx[2];
}

bool AdcMCP3564R::GetDataReady()
{
    uint16_t size = 2;

    // 0b01 0101 01
    uint8_t tx[size] = {kDeviceAddress | (kRegIRQ << 2)
                        | AdcMCP3564R::CommandType::STATIC_READ};

    uint8_t rx[size] = {0xff, 0xff};

    uint32_t timeout = 100;

    spi_handle_.BlockingTransmitAndReceive(tx, rx, size, timeout);

    uint8_t mask = 1 << 6;

    return (rx[1] & mask) == 0;
}

uint32_t AdcMCP3564R::Get(uint8_t idx)
{
    return val_[idx];
}

uint32_t* AdcMCP3564R::GetPtr(uint8_t idx)
{
    return &val_[idx];
}

void endCallback_(void* state, daisy::SpiHandle::Result res)
{
    uint8_t ch_idx, sign_bit;

    // TODO: tune this to guarantee the correct channel index
    // Also, check that the conversion to uint32_t is correct.
    for(size_t i = 1; i < 33; i += 4)
    {
        ch_idx   = spi_rx_buffer[i] >> 4;
        sign_bit = spi_rx_buffer[i] & 0xf;

        if(sign_bit)
        {
            channel_values[ch_idx] = 0;
        }
        else
        {
            channel_values[ch_idx] = spi_rx_buffer[i + 1] << 16
                                     | spi_rx_buffer[i + 2] << 8
                                     | spi_rx_buffer[i + 3];
        }
    }
}

daisy::SpiHandle::Result AdcMCP3564R::FetchConvertedDataDMA()
{
    uint8_t size     = 33;
    spi_tx_buffer[0] = kDeviceAddress | (kRegAdcData << 2)
                       | AdcMCP3564R::CommandType::STATIC_READ;

    daisy::SpiHandle::Result res = spi_handle_.DmaTransmitAndReceive(
        spi_tx_buffer, spi_rx_buffer, size, nullptr, endCallback_, nullptr);

    return res;
}