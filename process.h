#ifndef PROCESS_H
#define PROCESS_H

#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

#define PAGE_FLAGS (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE | PAGE_WRITECOPY)

class Process {
public:
    explicit Process(DWORD pid);
    ~Process();

    std::string name() const;
    HANDLE getHandle() const;

    std::vector<BYTE> readMemory(uintptr_t addr, size_t size) const;
    size_t writeMemory(HANDLE processHandle, uintptr_t addr, const std::vector<uint8_t>& value);

    std::vector<MEMORY_BASIC_INFORMATION> memoryRegions() const;
    std::vector<uint8_t> to_ne_bytes(int32_t value);

private:
    DWORD pid_;
    HANDLE handle_;
};

struct MemoryRegion {
    size_t address;
    int32_t value;
};

std::vector<DWORD> enumProc();
std::vector<MemoryRegion> exactValueScan(Process& proc, int32_t target);
std::vector<MemoryRegion> rangeValueScan(Process& proc, int32_t minValue, int32_t maxValue);

// New scanning functions
std::vector<MemoryRegion> increasedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions);
std::vector<MemoryRegion> increasedValueByScan(Process& proc, const std::vector<MemoryRegion>& previousRegions, int32_t amount);
std::vector<MemoryRegion> decreasedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions);
std::vector<MemoryRegion> decreasedValueByScan(Process& proc, const std::vector<MemoryRegion>& previousRegions, int32_t amount);
std::vector<MemoryRegion> changedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions);
std::vector<MemoryRegion> unchangedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions);

#endif // PROCESS_H
