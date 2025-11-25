#include "crash_log.h"
#include "sys/dma.h"

namespace fourseas
{

void CrashLogger::Init()
{
    ReadCrashCount();
}

void CrashLogger::ReadCrashCount()
{
    crash_count_ = 0;

    // Scan through crash log slots to count valid entries
    for(uint32_t i = 0; i < CRASH_LOG_MAX_COUNT; i++)
    {
        uint32_t   addr = GetCrashLogAddress(i);
        CrashLog* log  = (CrashLog*)qspi_.GetData(addr);

        // Invalidate cache before reading
        dsy_dma_invalidate_cache_for_buffer((uint8_t*)log, sizeof(CrashLog));

        if(log->magic == 0xDEADBEEF)
        {
            crash_count_++;
        }
        else
        {
            // Found empty slot, stop counting
            break;
        }
    }
}

void CrashLogger::SaveCrash(const CrashLog& log)
{
    // Find next available slot (circular buffer)
    uint32_t slot_index = crash_count_ % CRASH_LOG_MAX_COUNT;
    uint32_t addr       = GetCrashLogAddress(slot_index);

    // Erase 4KB sector for this crash log
    uint32_t erase_size = 4096;
    qspi_.Erase(addr, addr + erase_size);

    // Write crash log
    qspi_.Write(addr, sizeof(CrashLog), (uint8_t*)&log);

    crash_count_++;
}

const CrashLog* CrashLogger::GetCrashLog(uint32_t index) const
{
    if(index >= crash_count_)
    {
        return nullptr;
    }

    uint32_t addr = GetCrashLogAddress(index);
    CrashLog* log = (CrashLog*)qspi_.GetData(addr);

    // Invalidate cache before reading
    dsy_dma_invalidate_cache_for_buffer((uint8_t*)log, sizeof(CrashLog));

    return log;
}

void CrashLogger::ClearCrashes()
{
    // Erase entire crash log region
    qspi_.Erase(CRASH_LOG_BASE_ADDR, CRASH_LOG_BASE_ADDR + CRASH_LOG_SIZE);
    crash_count_ = 0;
}

uint32_t CrashLogger::GetCrashLogAddress(uint32_t index) const
{
    // Each crash log gets a 4KB sector
    return CRASH_LOG_BASE_ADDR + (index * 4096);
}

} // namespace fourseas
