#pragma once

#include "daisy.h"

class AdcMCP3564R
{
  public:
    enum class Result
    {
        OK,
        ERR,
    };

    enum ADCMode
    {
        ADC_CONVERT  = 0b11, // ADC Conversion mode
        ADC_STANDBY  = 0b10, // ADC Standby mode
        ADC_SHUTDOWN = 0b0,  // ADC Shutdown mode (default)
    };

    enum ConversionMode
    {
        // Continuous Conversion mode or continuous conversion cycle in SCAN mode.
        CONTINUOUS = 0b11,
        // One-shot conversion or one-shot cycle in SCAN mode. It sets ADC_MODE[1:0] to ‘10’ (standby) at
        // the end of the conversion or at the end of the conversion cycle in SCAN mode.
        ONE_SHOT_STBY = 0b10,
        // One-shot conversion or one-shot cycle in SCAN mode. It sets ADC_MODE[1:0] to ‘0x’ (ADC Shutdown)
        // at the end of the conversion or at the end of the conversion cycle in SCAN mode (default).
        ONE_SHOT_SHUTDOWN = 0b00,
    };

    enum ClkSel
    {
        // Internal clock is selected and AMCLK is present on the analog master clock output pin.
        INT_CLK_AMCLK = 0b11,
        //  = Internal clock is selected and no clock output is present on the CLK pin.
        INT_CLK_NO_OUT  = 0b10,
        EXT_CLK         = 0b01, //  = External digital clock
        EXT_CLK_DEFAULT = 0b00, //  = External digital clock (default)
    };

    enum AMCLKSel
    {
        MCLK_8       = 0b11, // AMCLK = MCLK/8
        MCLK_4       = 0b10, // AMCLK = MCLK/4
        MCLK_2       = 0b01, // AMCLK = MCLK/2
        MCLK_DEFAULT = 0b00, // AMCLK = MCLK (default)
    };

    enum OSRSel
    {
        OSR_98304 = 0b1111, // 1111 = OSR: 98304
        OSR_81920 = 0b1110, // 1110 = OSR: 81920
        OSR_49152 = 0b1101, // 1101 = OSR: 49152
        OSR_40960 = 0b1100, // 1100 = OSR: 40960
        OSR_24576 = 0b1011, // 1011 = OSR: 24576
        OSR_20480 = 0b1010, // 1010 = OSR: 20480
        OSR_16384 = 0b1001, // 1001 = OSR: 16384
        OSR_8192  = 0b1000, // 1000 = OSR: 8192
        OSR_4096  = 0b0111, // 0111 = OSR: 4096
        OSR_2048  = 0b0110, // 0110 = OSR: 2048
        OSR_1024  = 0b0101, // 0101 = OSR: 1024
        OSR_512   = 0b0100, // 0100 = OSR: 512
        OSR_256   = 0b0011, // 0011 = OSR: 256 (default)
        OSR_128   = 0b0010, // 0010 = OSR: 128
        OSR_64    = 0b0001, // 0001 = OSR: 64
        OSR_32    = 0b0000, // 0000 = OSR: 32
    };

    enum BoostSel
    {
        BOOST_2    = 0b11, // ADC channel has current x 2
        BOOST_1    = 0b10, // ADC channel has current x 1 (default)
        BOOST_0_66 = 0b01, // ADC channel has current x 0.66
        BOOST_0_5  = 0b00  // ADC channel has current x 0.5
    };

    enum DataFormats
    {
        DF_32_SIGNED_CHAN
        = 0b11, // 11 = 32-bit (25-bit right justified data + Channel ID): CHID[3:0] + SGN extension (4 bits) + 24-bit ADC data. It allows overrange with the SGN extension.
        DF_32_SIGNED
        = 0b10, // 10 = 32-bit (25-bit right justified data): SGN extension (8-bit) + 24-bit ADC data. It allows overrange with the SGN extension.
        DF_32_LEFT_PADDED
        = 0b01, // 01 = 32-bit (24-bit left justified data): 24-bit ADC data + 0x00 (8-bit). It does not allow overrange (ADC code locked to 0xFFFFFF or 0x800000).
        DF_24
        = 0b00 // 00 = 24-bit (default ADC coding): 24-bit ADC data. It does not allow overrange (ADC code locked to 0xFFFFFF or 0x800000).
    };

    enum ScanDelayTimes
    {

        SCAN_DELAY_512 = 0b111, // 512 * DMCLK
        SCAN_DELAY_256 = 0b110, // 256 * DMCLK
        SCAN_DELAY_128 = 0b101, // 128 * DMCLK
        SCAN_DELAY_64  = 0b100, // 64 * DMCLK
        SCAN_DELAY_32  = 0b011, // 32 * DMCLK
        SCAN_DELAY_16  = 0b010, // 16 * DMCLK
        SCAN_DELAY_8   = 0b001, // 8 * DMCLK
        SCAN_DELAY_0   = 0b000, // 0: No delay (default)
    };

    enum CommandType
    {
        FAST_CMD    = 0b00,
        STATIC_READ = 0b01,
        INCR_WRITE  = 0b10,
        INCR_READ   = 0b11,

    };

    enum ConfigRegister
    {
        CONFIG0 = 0b01,
        CONFIG1,
        CONFIG2,
        CONFIG3,
        CONFIG_LAST,
    };

    enum MuxMode
    {
        INT_VCM = 0b1111, // Internal VCM (1)
        // Internal Temperature Sensor Diode M (Temp Diode M)
        INT_TEMP_SENSOR_DIODE_M = 0b1110,
        // Internal Temperature Sensor Diode P (Temp Diode P)(1)
        INT_TEMP_SENSOR_DIODE_P = 0b1101,
        REFIN_INV               = 0b1100, // REFIN-
        REFIN_NON_INV           = 0b1011, // REFIN+/OUT
        AVDD                    = 0b1001, // AVDD
        AGND                    = 0b1000, // AGND
        CH7                     = 0b0111, // CH7
        CH6                     = 0b0110, // CH6
        CH5                     = 0b0101, // CH5
        CH4                     = 0b0100, // CH4
        CH3                     = 0b0011, // CH3
        CH2                     = 0b0010, // CH2
        CH1                     = 0b0001, // CH1
        CH0                     = 0b0000, // CH0
    };

    AdcMCP3564R() {}

    ~AdcMCP3564R() {}

    Result Init();

    Result ResetDefaults();

    uint8_t GetConfig(AdcMCP3564R::ConfigRegister reg);

    void UseInternalVRef(bool ref);

    void SetClock(AdcMCP3564R::ClkSel clk);

    void SetAMCLKPrescaler(AdcMCP3564R::AMCLKSel clkps);

    void SetOversamplingRatio(AdcMCP3564R::OSRSel sr);

    void SetBoost(AdcMCP3564R::BoostSel boost);

    void SetConversionMode(AdcMCP3564R::ConversionMode mode);

    void SetADCMode(AdcMCP3564R::ADCMode mode);

    void SetMuxReg(AdcMCP3564R::MuxMode msb, AdcMCP3564R::MuxMode lsb);

    void SetIRQPin();

    void SetDataFormat(AdcMCP3564R::DataFormats df);

    void SetScanRegister(AdcMCP3564R::ScanDelayTimes scan_delay,
                         uint16_t                    channels);

    uint32_t GetScanRegister();

    bool GetDataReady();

    daisy::SpiHandle::Result FetchConvertedDataDMA();

    uint32_t  Get(uint8_t idx);
    uint32_t* GetPtr(uint8_t idx);


  private:
    daisy::SpiHandle spi_handle_;

    uint32_t* val_;
};