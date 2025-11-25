#pragma once

#include <cstdint>
#include "per/qspi.h"

namespace fourseas
{

// QSPI memory map:
// 0x7FF000  - Serial number (4KB)
// 0x90000   - Settings
// 0x90000+  - App state
// 0x90010000 - Crash logs (start here, 64KB reserved = 16 crashes * 4KB/sector)
constexpr uint32_t CRASH_LOG_BASE_ADDR = 0x90010000;
constexpr uint32_t CRASH_LOG_SIZE      = 0x10000; // 64KB
constexpr uint32_t CRASH_LOG_MAX_COUNT = 16;      // Max crashes to store

struct CrashLog
{
    // Validation
    uint32_t magic; // 0xDEADBEEF = valid crash log

    // Timing
    uint32_t timestamp;     // System uptime in ms
    uint32_t reset_reason;  // RCC reset flags

    // CPU State (from exception stack frame)
    uint32_t r0, r1, r2, r3; // Arguments/scratch registers
    uint32_t r12;            // Scratch register
    uint32_t lr;             // Link register (return address)
    uint32_t pc;             // Program counter at fault
    uint32_t psr;            // Program status register

    // Fault Status Registers
    uint32_t cfsr; // Configurable Fault Status
    uint32_t hfsr; // HardFault Status
    uint32_t dfsr; // Debug Fault Status
    uint32_t afsr; // Auxiliary Fault Status
    uint32_t bfar; // Bus Fault Address
    uint32_t mmar; // MemManage Fault Address

    // Application context
    uint32_t audio_active; // Was audio callback running?

    // Stack dump for deeper analysis
    uint8_t stack_dump[64];

    // Padding to align to nice boundary
    uint8_t _padding[4];
};

static_assert(sizeof(CrashLog) == 140, "CrashLog size should be 140 bytes");

class CrashLogger
{
  public:
    CrashLogger(daisy::QSPIHandle& qspi) : qspi_(qspi), crash_count_(0) {}

    // Call during init to read existing crash count
    void Init();

    // Save a crash log (called from fault handler)
    void SaveCrash(const CrashLog& log);

    // Get number of crashes stored
    uint32_t GetCrashCount() const { return crash_count_; }

    // Get crash log by index (returns nullptr if invalid)
    const CrashLog* GetCrashLog(uint32_t index) const;

    // Clear all crash logs
    void ClearCrashes();

  private:
    daisy::QSPIHandle& qspi_;
    uint32_t           crash_count_;

    uint32_t GetCrashLogAddress(uint32_t index) const;
    void     ReadCrashCount();
};

} // namespace fourseas
