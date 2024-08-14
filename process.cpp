#include "process.h"

Process::Process(DWORD pid) 
    : pid_(pid), handle_(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid)) {
    if (GetLastError() == 5) {
        // Handle access denied error if needed
    } else {
        // Ignore other errors
    }
}

Process::~Process() {
    if (handle_ != NULL) {
        if (!CloseHandle(handle_)) {
            std::cerr << "Failed to close process handle. Error: " << GetLastError() << std::endl;
        }
    }
}

std::vector<uint8_t> Process::to_ne_bytes(int32_t value) {
    std::vector<uint8_t> bytes(sizeof(value));
    memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

std::string Process::name() const {
    HMODULE module;
    DWORD bytesReturned;
    if (!EnumProcessModules(handle_, &module, sizeof(module), &bytesReturned)) {
        return {};
    }

    char buffer[MAX_PATH];
    if (GetModuleBaseNameA(handle_, module, buffer, sizeof(buffer)) == 0) {
        std::cerr << "Failed to get module base name. Error: " << GetLastError() << std::endl;
        return {};
    }

    return std::string(buffer);
}

HANDLE Process::getHandle() const {
    return handle_;
}

std::vector<BYTE> Process::readMemory(uintptr_t addr, size_t size) const {
    std::vector<BYTE> buffer(size);
    SIZE_T bytesRead;
    if (ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    throw std::runtime_error("Failed to read memory");
}

size_t Process::writeMemory(HANDLE processHandle, uintptr_t addr, const std::vector<uint8_t>& value) {
    SIZE_T written = 0;
    if (!WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(addr), value.data(), value.size(), &written)) {
        throw std::runtime_error("Failed to write memory: " + std::to_string(GetLastError()));
    }
    return written;
}

std::vector<MEMORY_BASIC_INFORMATION> Process::memoryRegions() const {
    std::vector<MEMORY_BASIC_INFORMATION> regions;
    MEMORY_BASIC_INFORMATION mbi;
    LPCVOID address = nullptr;

    while (VirtualQueryEx(handle_, address, &mbi, sizeof(mbi))) {
        regions.push_back(mbi);
        address = (LPCVOID)((uintptr_t)mbi.BaseAddress + mbi.RegionSize);
    }
    return regions;
}

std::vector<DWORD> enumProc() {
    std::vector<DWORD> pids(1024); // Allocate space for 1024 PIDs
    DWORD bytesReturned;

    if (!EnumProcesses(pids.data(), pids.size() * sizeof(DWORD), &bytesReturned)) {
        std::cerr << "Failed to enumerate processes. Error: " << GetLastError() << std::endl;
        return {};
    }

    pids.resize(bytesReturned / sizeof(DWORD));
    return pids;
}

std::vector<MemoryRegion> increasedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions) {
    std::vector<MemoryRegion> newRegions;

    for (const auto& region : previousRegions) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t newValue;
            memcpy(&newValue, memory.data(), sizeof(int32_t));

            if (newValue > region.value) {
                newRegions.push_back({region.address, newValue});
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return newRegions;
}

std::vector<MemoryRegion> increasedValueByScan(Process& proc, const std::vector<MemoryRegion>& previousRegions, int32_t amount) {
    std::vector<MemoryRegion> newRegions;

    for (const auto& region : previousRegions) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t newValue;
            memcpy(&newValue, memory.data(), sizeof(int32_t));

            if (newValue == region.value + amount) {
                newRegions.push_back({region.address, newValue});
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return newRegions;
}

std::vector<MemoryRegion> decreasedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions) {
    std::vector<MemoryRegion> newRegions;

    for (const auto& region : previousRegions) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t newValue;
            memcpy(&newValue, memory.data(), sizeof(int32_t));

            if (newValue < region.value) {
                newRegions.push_back({region.address, newValue});
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return newRegions;
}

std::vector<MemoryRegion> decreasedValueByScan(Process& proc, const std::vector<MemoryRegion>& previousRegions, int32_t amount) {
    std::vector<MemoryRegion> newRegions;

    for (const auto& region : previousRegions) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t newValue;
            memcpy(&newValue, memory.data(), sizeof(int32_t));

            if (newValue == region.value - amount) {
                newRegions.push_back({region.address, newValue});
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return newRegions;
}

std::vector<MemoryRegion> changedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions) {
    std::vector<MemoryRegion> newRegions;

    for (const auto& region : previousRegions) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t newValue;
            memcpy(&newValue, memory.data(), sizeof(int32_t));

            if (newValue != region.value) {
                newRegions.push_back({region.address, newValue});
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return newRegions;
}

std::vector<MemoryRegion> unchangedValueScan(Process& proc, const std::vector<MemoryRegion>& previousRegions) {
    std::vector<MemoryRegion> newRegions;

    for (const auto& region : previousRegions) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t newValue;
            memcpy(&newValue, memory.data(), sizeof(int32_t));

            if (newValue == region.value) {
                newRegions.push_back({region.address, newValue});
            }
        } catch (...) {
            // Ignore errors
        }
    }

    return newRegions;
}

// Modify the existing scan functions to return MemoryRegion
std::vector<MemoryRegion> exactValueScan(Process& proc, int32_t target) {
    auto regions = proc.memoryRegions();
    std::vector<MEMORY_BASIC_INFORMATION> filteredRegions;
    std::vector<MemoryRegion> locations;

    for (const auto& region : regions) {
        if ((region.Protect & PAGE_FLAGS) != 0) {
            filteredRegions.push_back(region);
        }
    }

    std::vector<uint8_t> targetBytes = proc.to_ne_bytes(target);

    for (const auto& region : filteredRegions) {
        try {
            auto memory = proc.readMemory(reinterpret_cast<uintptr_t>(region.BaseAddress), region.RegionSize);

            for (size_t offset = 0; offset <= memory.size() - targetBytes.size(); offset += 4) {
                if (std::equal(targetBytes.begin(), targetBytes.end(), memory.begin() + offset)) {
                    int32_t foundValue;
                    memcpy(&foundValue, memory.data() + offset, sizeof(int32_t));
                    locations.push_back({reinterpret_cast<size_t>(region.BaseAddress) + offset, foundValue});
                }
            }

        } catch (const std::exception& err) {
            // Ignore Errors
        }
    }

    return locations;
}

std::vector<MemoryRegion> rangeValueScan(Process& proc, int32_t minValue, int32_t maxValue) {
    auto regions = proc.memoryRegions();
    std::vector<MemoryRegion> locations;
    
    // Filter regions based on memory protection flags
    std::vector<MEMORY_BASIC_INFORMATION> filteredRegions;
    for (const auto& region : regions) {
        if ((region.Protect & PAGE_FLAGS) != 0) {
            filteredRegions.push_back(region);
        }
    }

    for (const auto& region : filteredRegions) {
        try {
            auto memory = proc.readMemory(reinterpret_cast<uintptr_t>(region.BaseAddress), region.RegionSize);

            for (size_t offset = 0; offset <= memory.size() - sizeof(int32_t); offset += 4) {
                int32_t value;
                memcpy(&value, memory.data() + offset, sizeof(int32_t));

                // Check if the value is within the specified range
                if (value >= minValue && value <= maxValue) {
                    locations.push_back({reinterpret_cast<size_t>(region.BaseAddress) + offset, value});
                }
            }
        } catch (const std::exception& err) {
            // Ignore Errors
        }
    }

    // Performing the second scan on the found locations
    std::vector<MemoryRegion> newLocations;
    for (auto& region : locations) {
        try {
            auto memory = proc.readMemory(region.address, sizeof(int32_t));
            int32_t value;
            memcpy(&value, memory.data(), sizeof(int32_t));

            if (value >= minValue && value <= maxValue) {
                newLocations.push_back({reinterpret_cast<size_t>(region.address), region.value});
            }
        } catch (...) {
            // Ignore errors for now
        }
    }

    return newLocations;
}
