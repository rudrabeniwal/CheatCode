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

std::vector<DWORD> enumProc();
std::vector<size_t> exactValueScan(Process& proc, int32_t target);

#endif // PROCESS_H
