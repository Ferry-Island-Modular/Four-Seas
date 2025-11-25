#include "fourSeasBoard.h"

#include "daisy.h"
#include "daisy_seed.h"
#include "dev/codec_ak4556.h"

#include "src/drivers/MCP3564R.h"

namespace fourseas
{
// Pots and mux
constexpr daisy::Pin PIN_MUX_POTS_0 = daisy::seed::A3;  // Seed2 DFM C4
constexpr daisy::Pin PIN_MUX_POTS_1 = daisy::seed::A1;  // Seed2 DFM C1
constexpr daisy::Pin PIN_MUX_ADDR_0 = daisy::seed::D32; // Seed2 DFM D8
constexpr daisy::Pin PIN_MUX_ADDR_1 = daisy::seed::D0;  // Seed2 DFM D9
constexpr daisy::Pin PIN_MUX_ADDR_2 = daisy::seed::D31; // Seed2 DFM C10

// I2C pins
constexpr daisy::Pin PIN_I2C1_SCL = daisy::seed::D11; // // Seed2 DFM B7
constexpr daisy::Pin PIN_I2C1_SDA = daisy::seed::D12; // // Seed2 DFM B8

constexpr daisy::Pin PIN_I2C4_SCL = daisy::seed::D13; // // Seed2 DFM B3
constexpr daisy::Pin PIN_I2C4_SDA = daisy::seed::D14; // // Seed2 DFM B5

// ADC ready IRQ pin
constexpr daisy::Pin PIN_ADC_READY_IRQ = daisy::seed::D20; // Seed2 DFM C3

// ADC direct in (pots or CVs)
constexpr daisy::Pin PIN_ADC_CV0 = daisy::seed::A4; // Seed2 DFM C2 voct
constexpr daisy::Pin PIN_ADC_CV1 = daisy::seed::A7; // Seed2 DFM C9 bcv
constexpr daisy::Pin PIN_ADC_CV2 = daisy::seed::A2; // Seed2 DFM C5 spre
constexpr daisy::Pin PIN_ADC_CV3 = daisy::seed::A8; // Seed2 DFM C8 bpot

// I2C addresses
constexpr uint8_t LED_DRIVER_I2C_ADDR_1 = 0x00;
constexpr uint8_t LED_DRIVER_I2C_ADDR_2 = 0x01;
constexpr uint8_t LED_DRIVER_I2C_ADDR_3 = 0x02;
constexpr uint8_t EXT_I2C_ADDR          = 0x11;


// ADC data ready IRQ pin
#define ADC_READY_Pin GPIO_PIN_1
#define ADC_READY_GPIO_Port GPIOC


void FourSeasHW::Init(bool boost)
{
    // seed init
    seed.Init(boost);

    InitAudio();

    // This applies settings to both CODECs
    seed.SetAudioBlockSize(24);
    seed.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);

    InitAnalogInputs();
    InitExtADC();
    InitADCCVs();
    InitExtI2C();

    // Lower peripheral priorities to not collide with audio callback
    HAL_NVIC_SetPriority(SPI1_IRQn, 1, 1);
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 1, 1);


#ifdef TESTING_FIRMWARE
    // InitLEDDriver();  // Commented out for testing
    // InitExtGPIO();    // Commented out for testing
#else
    InitLEDDriver();
    InitExtGPIO();
#endif
}

void FourSeasHW::SetHidUpdateRates()
{
    for(size_t i = 0; i < KNOB_LAST; i++)
    {
        knobs[i].SetSampleRate(AudioCallbackRate());
    }
}

void FourSeasHW::DelayMs(size_t del)
{
    seed.DelayMs(del);
}

uint32_t FourSeasHW::GetTick()
{
    return seed.system.GetTick();
}

uint32_t FourSeasHW::GetNow()
{
    return seed.system.GetNow();
}

void FourSeasHW::InitWatchdog()
{
    hiwdg_.Instance       = IWDG1;
    hiwdg_.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg_.Init.Reload    = 1875; // 15 second timeout
    hiwdg_.Init.Window    = IWDG_WINDOW_DISABLE;

    HAL_IWDG_Init(&hiwdg_);
}

void FourSeasHW::RefreshWatchdog()
{
    HAL_IWDG_Refresh(&hiwdg_);
}

void FourSeasHW::StartAudio(daisy::AudioHandle::InterleavingAudioCallback cb)
{
    seed.StartAudio(cb);
}

void FourSeasHW::StartAudio(daisy::AudioHandle::AudioCallback cb)
{
    seed.StartAudio(cb);
}

void FourSeasHW::ChangeAudioCallback(
    daisy::AudioHandle::InterleavingAudioCallback cb)
{
    seed.ChangeAudioCallback(cb);
}

void FourSeasHW::ChangeAudioCallback(daisy::AudioHandle::AudioCallback cb)
{
    seed.ChangeAudioCallback(cb);
}

void FourSeasHW::StopAudio()
{
    seed.StopAudio();
}

void FourSeasHW::SetAudioBlockSize(size_t size)
{
    seed.SetAudioBlockSize(size);
    SetHidUpdateRates();
}

size_t FourSeasHW::AudioBlockSize()
{
    return seed.AudioBlockSize();
}

void FourSeasHW::SetAudioSampleRate(
    daisy::SaiHandle::Config::SampleRate samplerate)
{
    seed.SetAudioSampleRate(samplerate);
    SetHidUpdateRates();
}

float FourSeasHW::AudioSampleRate()
{
    return seed.AudioSampleRate();
}

float FourSeasHW::AudioCallbackRate()
{
    return seed.AudioCallbackRate();
}

void FourSeasHW::StartAdc()
{
    seed.adc.Start();
}

void FourSeasHW::StopAdc()
{
    seed.adc.Stop();
}

void FourSeasHW::ProcessAnalogControls()
{
    for(auto &knob : knobs)
    {
        knob.Process();
    }

    cv[ONBOARD_CV_0].Process();
    cv[ONBOARD_CV_1].Process();

    for(auto &adc_cv : adc_cvs)
    {
        adc_cv.Process();
    }
}

void FourSeasHW::UpdateLEDs()
{
    led_driver.Transmit();
}

void FourSeasHW::InitLEDDriver()
{
    led_driver.Init();
}

void FourSeasHW::UpdateExtGPIO()
{
    uint8_t state = ext_gpio_.Fetch();

    for(size_t i = 0; i < BUTTON_LAST; i++)
    {
        uint16_t *button_state = buttons[i].GetState();

        *button_state = *button_state << 1;
        if(state & (1 << i))
        {
            *button_state |= 1;
        }
        buttons[i].UpdateTimestamp();
    }
}

daisy::SpiHandle::Result FourSeasHW::UpdateExtADC()
{
    return adc_.FetchConvertedDataDMA();
}

void FourSeasHW::InitAnalogInputs()
{
    daisy::AdcChannelConfig adc_init[ANALOG_LAST];

    adc_init[MUX_0].InitMux(
        PIN_MUX_POTS_0, 8, PIN_MUX_ADDR_0, PIN_MUX_ADDR_1, PIN_MUX_ADDR_2);

    adc_init[MUX_1].InitMux(
        PIN_MUX_POTS_1, 8, PIN_MUX_ADDR_0, PIN_MUX_ADDR_1, PIN_MUX_ADDR_2);

    adc_init[CV_0].InitSingle(PIN_ADC_CV0);          // 1/v oct
    adc_init[CV_1].InitSingle(PIN_ADC_CV1);          // bank CV
    adc_init[ROTARY_SWITCH].InitSingle(PIN_ADC_CV2); // spread
    adc_init[KNOB_DIRECT].InitSingle(PIN_ADC_CV3);   // bpot

    seed.adc.Init(adc_init, ANALOG_LAST, daisy::AdcHandle::OVS_128);

    // Setup all 16 mux knobs
    for(size_t i = 0; i < 2; i++)
    {
        for(size_t j = 0; j < 8; j++)
        {
            knobs[j + (i * 8)].Init(seed.adc.GetMuxPtr(i, j),
                                    AudioCallbackRate());
        }
    }

    // Setup CV ins
    // TODO: iterate this
    cv[ONBOARD_CV_0].InitBipolarCv(seed.adc.GetPtr(CV_0), AudioCallbackRate());
    cv[ONBOARD_CV_1].InitBipolarCv(seed.adc.GetPtr(CV_1), AudioCallbackRate());
    rotary_switch.Init(seed.adc.GetPtr(ROTARY_SWITCH), AudioCallbackRate());
    extra_knob.Init(seed.adc.GetPtr(KNOB_DIRECT), AudioCallbackRate());

    cv[ONBOARD_CV_0].SetScale(2.0f);
    cv[ONBOARD_CV_1].SetScale(2.0f);
}

void FourSeasHW::InitADCCVs()
{
    for(size_t i = 0; i < ADC_CV_LAST; i++)
    {
        adc_cvs[i].InitBipolarCv(adc_.GetPtr(i), AudioCallbackRate());
    }
}

void FourSeasHW::InitAudio()
{
    /** Configure the SAI2 peripheral for our secondary codec. */
    daisy::SaiHandle         external_sai_handle;
    daisy::SaiHandle::Config external_sai_cfg;
    external_sai_cfg.periph = daisy::SaiHandle::Config::Peripheral::SAI_2;
    external_sai_cfg.sr     = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
    external_sai_cfg.bit_depth = daisy::SaiHandle::Config::BitDepth::SAI_24BIT;
    external_sai_cfg.a_sync    = daisy::SaiHandle::Config::Sync::SLAVE;
    external_sai_cfg.b_sync    = daisy::SaiHandle::Config::Sync::MASTER;
    external_sai_cfg.a_dir     = daisy::SaiHandle::Config::Direction::TRANSMIT;
    external_sai_cfg.b_dir     = daisy::SaiHandle::Config::Direction::RECEIVE;
    external_sai_cfg.pin_config.fs   = daisy::seed::D27;
    external_sai_cfg.pin_config.mclk = daisy::seed::D24;
    external_sai_cfg.pin_config.sck  = daisy::seed::D28;
    external_sai_cfg.pin_config.sb   = daisy::seed::D25;
    external_sai_cfg.pin_config.sa   = daisy::seed::D26;

    /** Initialize the SAI new handle */
    external_sai_handle.Init(external_sai_cfg);

    /** Reconfigure Audio for two codecs
     *
     *  Default eurorack circuit has an extra 6dB headroom
     *  so the 0.5 here makes it so that a -1 to 1 audio signal
     *  will correspond to a -5V to 5V (10Vpp) audio signal.
     *  Audio will clip at -2 to 2, and result 20Vpp output.
     */
    daisy::AudioHandle::Config audio_cfg;
    audio_cfg.blocksize  = 48;
    audio_cfg.samplerate = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
    audio_cfg.postgain   = 1.0f;

    seed.audio_handle.Init(
        audio_cfg, seed.AudioSaiHandle(), external_sai_handle);
}

void FourSeasHW::InitExtADC()
{
    adc_.Init();

    adc_.ResetDefaults();
    adc_.UseInternalVRef(false);

    adc_.SetClock(AdcMCP3564R::ClkSel::EXT_CLK);

    adc_.SetConversionMode(AdcMCP3564R::ConversionMode::CONTINUOUS);
    adc_.SetADCMode(AdcMCP3564R::ADCMode::ADC_CONVERT);

    adc_.SetBoost(AdcMCP3564R::BoostSel::BOOST_2);

    // OSR_1024 IRQs at ~2.4k and is about the max we can handle
    // OSR_2048 clocks @ 1.6k if we want some breathing room
    // OSR_256 seems to work as well, need to investigate how often this is getting dropped
    // or if it works okay. Clocks @ ~6.5kk

    // OSR_32 max: 153600, actual: ~51.1kHz
    // OSR_256 max: 19200, actual: ~6.4kHz
    // OSR_512 max: 9600, actual: ~3.2kHz
    // OSR_1024 max: 4800, actual: ~2.4kHz

    adc_.SetOversamplingRatio(AdcMCP3564R::OSRSel::OSR_2048);

    adc_.SetDataFormat(AdcMCP3564R::DataFormats::DF_32_SIGNED_CHAN);

    adc_.SetScanRegister(AdcMCP3564R::ScanDelayTimes::SCAN_DELAY_0, 0xff);

    // Setup ADC IRQ pinout
    // TODO: wrap these naked HAL calls to make more idiomatic
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin  = ADC_READY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ADC_READY_GPIO_Port, &GPIO_InitStruct);
}

void FourSeasHW::InitExtGPIO()
{
    ext_gpio_.Init(0b0100000);
}

void FourSeasHW::InitExtI2C()
{
    daisy::I2CHandle::Config i2c_config;

    i2c_config.periph         = daisy::I2CHandle::Config::Peripheral::I2C_4;
    i2c_config.speed          = daisy::I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_config.mode           = daisy::I2CHandle::Config::Mode::I2C_SLAVE;
    i2c_config.address        = EXT_I2C_ADDR;
    i2c_config.pin_config.scl = PIN_I2C4_SCL;
    i2c_config.pin_config.sda = PIN_I2C4_SDA;

    ext_i2c_handle_.Init(i2c_config);
}

} // namespace fourseas
