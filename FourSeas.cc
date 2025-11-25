#include "fatfs.h"

#include "wavetable_oscillator.h"
#include "src/hardware/fourSeasBoard.h"
#include "src/settings.h"
#include "src/ui.h"
#include "src/crash_log.h"

using namespace daisy;
using namespace daisysp;
using namespace fourseas;


// ============================================================================
// Constants
// ============================================================================
static constexpr size_t kNumWaves = 8;
static constexpr size_t kNumCols  = 8;
static constexpr size_t kNumPages = 8;
static constexpr size_t kNumBanks = 12;

static constexpr size_t kNumWaveSamples =
#ifdef CURRENT_WAVE_SAMPLES
    CURRENT_WAVE_SAMPLES;
#else
    2048;
#endif

static constexpr uint8_t kWtAudio1 = Ui::WT_OSCS::WT_AUDIO_1;
static constexpr uint8_t kWtAudio2 = Ui::WT_OSCS::WT_AUDIO_2;
static constexpr uint8_t kWtAudio3 = Ui::WT_OSCS::WT_AUDIO_3;
static constexpr uint8_t kWtAudio4 = Ui::WT_OSCS::WT_AUDIO_4;

static constexpr uint8_t kWtSyncA = Ui::WT_OSCS::WT_AUDIO_1;
static constexpr uint8_t kWtSyncB = Ui::WT_OSCS::WT_AUDIO_2;
static constexpr uint8_t kWtModA  = Ui::WT_OSCS::WT_AUDIO_3;
static constexpr uint8_t kWtModB  = Ui::WT_OSCS::WT_AUDIO_4;

// ============================================================================
// Hardware & Peripherals
// ============================================================================
static FourSeasHW hw;
static Ui         ui;

static FatFSInterface       fsi;
static SdmmcHandler         sdcard;
static SdmmcHandler::Config sd_config;
static FIL                  SDFile;

static TimerHandle timer_handle;

// Must be non-static (accessed from extern "C" callback)
SpiHandle::Result adc_res = SpiHandle::Result::OK;

// External HAL handle (defined in libDaisy)
extern SD_HandleTypeDef hsd1;

// ============================================================================
// Synthesis Engine
// ============================================================================
static WavetableOscillator<kNumWaveSamples, true, true>
    wto_full[2]; // A1, B1 - full features
static WavetableOscillator<kNumWaveSamples, false, false>
    wto_basic[2]; // A2, B2 - basic only

static float          wt_outs[Ui::WT_OSCS::WT_OSCS_LAST];
static Params::Values vals[4];

// ============================================================================
// Wavetable Storage (SDRAM)
// ============================================================================
static WaveTableLoader wtl;

static float DSY_SDRAM_BSS __attribute__((aligned(
    32))) table[kNumBanks * kNumPages * kNumWaveSamples * kNumWaves * kNumCols];

alignas(32) static float* DSY_SDRAM_BSS
    wave_offsets[kNumWaves * kNumCols * kNumPages * kNumBanks];

// ============================================================================
// Persistent Settings
// ============================================================================
static Settings                        settings;
static PersistentStorage<SettingsData> settings_storage(hw.seed.qspi);
static AppState                        app_state;
static PersistentStorage<AppState>     app_state_storage(hw.seed.qspi);

// ============================================================================
// Crash Logging
// ============================================================================
static CrashLogger crash_logger(hw.seed.qspi);

// Track if audio is running (for crash context)
static volatile bool audio_active = false;

// ============================================================================
// Audio Callback & Helper Functions
// ============================================================================

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    audio_active = true;

    // input (should) scale from -1 to 1.
    // 10vpp yields -0.640790105 to 0.64958632
    // this gives us some headroom for louder signals before clipping
    ui.UpdateParams(); // get current UI x and init all 4 interpolators with it

    bool interpolate = ui.state_->interpolate_waves;
    bool osc_sync_1  = false;
    bool osc_sync_2  = false;

    // Audio Loop
    for(size_t i = 0; i < size; i++)
    {
        // GetParams returns interpolated data for each parameter in one call
        // It should be called once per sample
        vals[kWtAudio4] = ui.GetParams(kWtAudio4);
        vals[kWtAudio3] = ui.GetParams(kWtAudio3);
        vals[kWtAudio1] = ui.GetParams(kWtAudio1);
        vals[kWtAudio2] = ui.GetParams(kWtAudio2);

        osc_sync_1 = (in[kWtSyncA][i] > 0.05f);
        osc_sync_2 = (in[kWtSyncB][i] > 0.05f);

        OscillatorParams osc_params[4] = {
            {vals[kWtAudio1],
             interpolate,
             ui.state_->mod_state_1,
             in[kWtModA][i],
             ui.state_->sync_mode_1,
             osc_sync_1},

            {vals[kWtAudio3],
             interpolate,
             ui.state_->mod_state_2,
             in[kWtModB][i],
             ui.state_->sync_mode_2,
             osc_sync_2},

            {vals[kWtAudio2], interpolate},
            {vals[kWtAudio4], interpolate},
        };

        // Full-featured oscillators (compile ALL features)
        wto_full[0].Render(osc_params[0], &wt_outs[0]); // A1
        wto_full[1].Render(osc_params[1], &wt_outs[1]); // B1

        // Basic oscillators (compile NO mod/sync code)
        wto_basic[0].Render(osc_params[2], &wt_outs[2]); // A2
        wto_basic[1].Render(osc_params[3], &wt_outs[3]); // B2

        // Process
        out[0][i] = wt_outs[kWtAudio4]; // B2
        out[1][i] = wt_outs[kWtAudio2]; // B1
        out[2][i] = wt_outs[kWtAudio1];

        // A1 = 0, B1 = 1, A2 = 2, B2 = 3
        // 1+2 are onboard
        // 3+4 are external
        // but! ext is rendered first due to weirdness in libDaisy
        // so really:
        // 1 = out[2]
        // 2 = out[3]
        // 3 = out[1]
        // 4 = out[0]

        if constexpr(fourseas::FourSeasHW::kCurrentBoardRevVar
                     == FourSeasHW::BoardRevision::REV_3)
        {
            out[3][i] = wt_outs[kWtAudio3] * -1.0f; // A2
        }
        else
        {
            out[3][i] = wt_outs[kWtAudio3]; // A2
        }
    }
}

static void InitSynth()
{
    for(auto& osc : wto_full)
    {
        osc.Init(wave_offsets);
    }

    for(auto& osc : wto_basic)
    {
        osc.Init(wave_offsets);
    }
}

// Returns true on success, false on error
static bool InitSDCardFileSystem()
{
    sd_config.Defaults();

    if constexpr(hw.kCurrentBoardRevVar == FourSeasHW::BoardRevision::REV_3)
    {
        sd_config.speed = SdmmcHandler::Speed::STANDARD;
    }
    // TODO: FAST still does not work.
    sd_config.speed = SdmmcHandler::Speed::STANDARD;

    if(sdcard.Init(sd_config) != SdmmcHandler::Result::OK)
    {
        return false;
    }

    if(fsi.Init(FatFSInterface::Config::MEDIA_SD) != FatFSInterface::Result::OK)
    {
        return false;
    }

    FATFS& fs = fsi.GetSDFileSystem();
    if(f_mount(&fs, "/", 1) != FR_OK)
    {
        return false;
    }

    return true;
}

// Returns true on success, false on fatal error
// Note: Partial success (at least 1 bank loaded) returns true
static bool LoadWavetables()
{
    ui.StartupLEDSequence();

    char          filename[20];
    const uint8_t kMaxRetries = 3;
    size_t        bank_offset = kNumPages * kNumWaves * kNumCols;

    for(size_t bank_idx = 0; bank_idx < kNumBanks; bank_idx++)
    {
        hw.RefreshWatchdog();
        ui.FadeBankLED(bank_idx);

        for(size_t page_idx = 0; page_idx < kNumPages; page_idx++)
        {
            snprintf(filename,
                     sizeof(filename),
                     "/%d/%d.wav",
                     bank_idx + 1,
                     page_idx + 1);

            wtl.Init(
                &table[(bank_idx * bank_offset * kNumWaveSamples)
                       + page_idx * kNumWaveSamples * kNumWaves * kNumCols],
                kNumWaveSamples * kNumWaves * kNumCols);
            wtl.SetWaveTableInfo(kNumWaveSamples, kNumWaves * kNumCols);

            WaveTableLoader::Result res = WaveTableLoader::Result::ERR_GENERIC;

            // Retry load with delay between attempts
            for(uint8_t retry = 0;
                retry < kMaxRetries && res != WaveTableLoader::Result::OK;
                retry++)
            {
                res = wtl.Import(filename);
                if(res != WaveTableLoader::Result::OK)
                {
                    hw.DelayMs(50);
                }
            }

            if(res == WaveTableLoader::Result::OK)
            {
                ui.SetBanksMax(bank_idx + 1);

                for(size_t j = 0; j < kNumWaves * kNumCols; j++)
                {
                    wave_offsets[((bank_idx * bank_offset)
                                  + page_idx * kNumWaves * kNumCols)
                                 + j]
                        = wtl.GetTable(j);
                }
            }
            else if(bank_idx > 0)
            {
                // Partial success - at least one bank loaded
                return true;
            }
            else
            {
                // Fatal error - no banks loaded, show red LEDs
                for(uint8_t i = 0; i < 3; i++)
                {
                    ui.FadeToColor(255, 0, 0, 1000);
                }
                return false;
            }
        }
    }
    return true;
}

static void GPIOTimerCB(void* data)
{
    hw.UpdateExtGPIO();
}


static void InitTimer()
{
    uint32_t tim_base_freq = System::GetPClk2Freq();
    uint32_t target_freq   = 320;
    uint32_t period        = tim_base_freq / target_freq;

    TimerHandle::Config tconf;
    tconf.dir        = TimerHandle::Config::CounterDir::UP;
    tconf.enable_irq = true;
    tconf.periph     = TimerHandle::Config::Peripheral::TIM_5;
    tconf.period     = period;

    timer_handle.Init(tconf);
    timer_handle.SetCallback(GPIOTimerCB);
    timer_handle.Start();
}

extern "C"
{
    // Called from HardFault_Handler with stack frame
    // Stack frame layout (pushed automatically by Cortex-M on exception):
    // SP+0:  R0, SP+4:  R1, SP+8:  R2, SP+12: R3
    // SP+16: R12, SP+20: LR, SP+24: PC, SP+28: xPSR
    void UserHardFaultIndicator(uint32_t* stack_frame)
    {
        CrashLog log;

        // Initialize with magic number
        log.magic = 0xDEADBEEF;

        // Capture timing and reset reason
        log.timestamp    = System::GetNow();
        log.reset_reason = RCC->RSR; // Reset status register

        // Capture CPU state from exception stack frame
        log.r0  = stack_frame[0];
        log.r1  = stack_frame[1];
        log.r2  = stack_frame[2];
        log.r3  = stack_frame[3];
        log.r12 = stack_frame[4];
        log.lr  = stack_frame[5];
        log.pc  = stack_frame[6];
        log.psr = stack_frame[7];

        // Capture fault status registers
        log.cfsr = SCB->CFSR;
        log.hfsr = SCB->HFSR;
        log.dfsr = SCB->DFSR;
        log.afsr = SCB->AFSR;
        log.bfar = SCB->BFAR;
        log.mmar = SCB->MMFAR;

        // Application context
        log.audio_active = audio_active ? 1 : 0;

        // Capture stack dump (64 bytes from current stack)
        for(size_t i = 0; i < 64; i++)
        {
            log.stack_dump[i] = ((uint8_t*)stack_frame)[i];
        }

        // Save to QSPI flash
        crash_logger.SaveCrash(log);

        // Infinite loop - let watchdog reset us
        while(true)
        {
            __NOP();
        }
    }

    void HAL_GPIO_EXTI_Callback(uint16_t /*GPIO_Pin*/)
    {
        adc_res = hw.UpdateExtADC();
    }

    void EXTI1_IRQHandler()
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    }
}

void TriggerTestCrash()
{
    // Dereference NULL pointer - guaranteed hard fault
    volatile uint32_t* bad_ptr = nullptr;
    *bad_ptr                   = 0xDEAD;
}

auto main() -> int
{
    hw.Init(true);

    // Initialize crash logger to read existing crash count
    crash_logger.Init();

    // Check if we reset due to watchdog (fault occurred)
    if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST))
    {
        __HAL_RCC_CLEAR_RESET_FLAGS();

        // Initialize just enough for LED display
        ui.Init(&hw, &settings_storage, &app_state_storage);

        // Show crash count by blinking N times, then blink red continuously
        uint32_t crash_count = crash_logger.GetCrashCount();

        // Blink white N times to indicate crash count
        for(uint32_t i = 0; i < crash_count; i++)
        {
            ui.SetLEDsOn();
            hw.DelayMs(300);
            ui.SetLEDsOff();
            hw.DelayMs(300);
        }

        // Then blink red forever (no watchdog this time)
        while(true)
        {
            ui.SetLEDsOff();
            hw.DelayMs(500);
            ui.SetLEDsRed();
            hw.DelayMs(500);
        }
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();

    //hw.InitWatchdog();

    // Flush-to-zero mode for denormal
    FPU->FPDSCR |= FPU_FPDSCR_FZ_Msk;

    settings_storage.Init(settings.Default());
    app_state_storage.Init(app_state.Default(), 0x90000);

    //settingsStorage.RestoreDefaults();
    //appStateStorage.RestoreDefaults();

    ui.Init(&hw, &settings_storage, &app_state_storage);

    // Initialize SD card and dump crash logs (if any)
    if(InitSDCardFileSystem())
    {
        // Dump crash logs to SD if any exist
        if(crash_logger.GetCrashCount() > 0)
        {
            FRESULT  f_res;
            uint32_t crash_count = crash_logger.GetCrashCount();

            f_res
                = f_open(&SDFile, "/crashes.bin", FA_WRITE | FA_CREATE_ALWAYS);
            if(f_res == FR_OK)
            {
                for(uint32_t i = 0; i < crash_count; i++)
                {
                    hw.RefreshWatchdog();

                    const CrashLog* log = crash_logger.GetCrashLog(i);
                    if(log != nullptr)
                    {
                        UINT    written;
                        FRESULT write_res
                            = f_write(&SDFile, log, sizeof(CrashLog), &written);
                        if(write_res != FR_OK || written != sizeof(CrashLog))
                        {
                            break;
                        }
                    }
                }

                hw.RefreshWatchdog();
                f_res = f_close(&SDFile);
            }
        }

        if(LoadWavetables())
        {
            InitSynth();
            hw.StartAudio(AudioCallback);
            ui.SetWavesLoaded(true);
        }
        else
        {
            ui.SetWavesLoaded(false);
        }
    }
    else
    {
        ui.SetWavesLoaded(false);
    }

    InitTimer();
    hw.StartAdc();

    /* EXTI interrupt init for ADC data ready timer */
    HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    uint8_t last_bank = 0;

    // crash here - uncomment as needed for testing / debugging
    // TriggerTestCrash();

    while(true)
    {
        hw.RefreshWatchdog();

        bool    freshly_calibrated = ui.Process();
        uint8_t bank               = ui.GetBankNum();
        if(bank != last_bank)
        {
            for(auto& osc : wto_full)
            {
                osc.SetBank(bank);
            }

            for(auto& osc : wto_basic)
            {
                osc.SetBank(bank);
            }
            last_bank = bank;
        }

        // Hot reload wavetables when both LFO toggle buttons held >2s
        if(hw.buttons[Ui::SW_LFO_TOGGLE_1].TimeHeldMs() > 2000
           && hw.buttons[Ui::SW_LFO_TOGGLE_2].TimeHeldMs() > 2000)
        {
            hw.StopAudio();

            // Unmount and deinit SD card hardware
            f_mount(nullptr, "/", 1);
            if(fsi.Initialized())
            {
                fsi.DeInit();
            }
            HAL_SD_DeInit(&hsd1);

            // Reinitialize and reload
            if(InitSDCardFileSystem())
            {
                if(LoadWavetables())
                {
                    InitSynth();
                    hw.StartAudio(AudioCallback);
                    ui.SetWavesLoaded(true);
                }
                else
                {
                    ui.SetWavesLoaded(false);
                }
            }
            else
            {
                ui.SetWavesLoaded(false);
            }
        }

        if(freshly_calibrated)
        {
            ui.Init(&hw, &settings_storage, &app_state_storage);
        }
    }
}
