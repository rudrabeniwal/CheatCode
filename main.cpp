#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cstdint>

#define PAGE_FLAGS (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE | PAGE_WRITECOPY)

class Process {
public:
    explicit Process(DWORD pid) 
        : pid_(pid), handle_(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid)) {
        if ( GetLastError() == 5){
            } else{
            //ignore
            }
    }

    ~Process() {
        if (handle_ != NULL) {
            if (!CloseHandle(handle_)) {
                std::cerr << "Failed to close process handle. Error: " << GetLastError() << std::endl;
            }
        }
    }

    // Converts a 32-bit integer to a byte array in native endianness
    std::vector<uint8_t> to_ne_bytes(int32_t value) {
        std::vector<uint8_t> bytes(sizeof(value));
        memcpy(bytes.data(), &value, sizeof(value));
        return bytes;
    }

    std::string name() const {
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
    
    HANDLE getHandle() const {
        return handle_;
    }

    std::vector<BYTE> readMemory(uintptr_t addr, size_t size) const {
        std::vector<BYTE> buffer(size);
        SIZE_T bytesRead;
        if (ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), buffer.data(), size, &bytesRead)) {
            buffer.resize(bytesRead);
            return buffer;
        }
        throw std::runtime_error("Failed to read memory");
    }

    size_t writeMemory(HANDLE processHandle, uintptr_t addr, const std::vector<uint8_t>& value) {
    SIZE_T written = 0;
    if (!WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(addr), value.data(), value.size(), &written)) {
        throw std::runtime_error("Failed to write memory: " + std::to_string(GetLastError()));
    }
    return written;
}

    std::vector<MEMORY_BASIC_INFORMATION> memoryRegions() const {
        std::vector<MEMORY_BASIC_INFORMATION> regions;
        MEMORY_BASIC_INFORMATION mbi;
        LPCVOID address = nullptr;

        while (VirtualQueryEx(handle_, address, &mbi, sizeof(mbi))) {
            regions.push_back(mbi);
            address = (LPCVOID)((uintptr_t)mbi.BaseAddress + mbi.RegionSize);
        }
        return regions;
    }

private:
    DWORD pid_;
    HANDLE handle_;
};

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

int main() {
    auto pids = enumProc();
    int success = 0;
    int failed = 0;
    for (DWORD pid : pids) {
        Process proc(pid);
        if (proc.getHandle() != NULL) {
            std::string name = proc.name();
            if (!name.empty()) {
                ++success;

                // Calculate the total memory size excluding PAGE_NOACCESS
                auto regions = proc.memoryRegions();
                std::vector<MEMORY_BASIC_INFORMATION> filteredRegions;

                for (const auto& region : regions) {
                    if ((region.Protect & PAGE_FLAGS) != 0) {
                        filteredRegions.push_back(region);
                    }
                }
                int32_t target = 73571783; // The value we're looking for
                std::vector<uint8_t> targetBytes = proc.to_ne_bytes(target); // Converting target to byte array
                std::vector<size_t> locations;
             
                for (const auto& region : filteredRegions) {
                    
                    try {
                        auto memory = proc.readMemory(reinterpret_cast<uintptr_t>(region.BaseAddress), region.RegionSize);
                        
                        for (size_t offset = 0; offset <= memory.size() - targetBytes.size(); offset += 4) {
                            
                            if (std::equal(targetBytes.begin(), targetBytes.end(), memory.begin() + offset)) {
                                locations.push_back(reinterpret_cast<size_t>(region.BaseAddress) + offset);
                            }
                        }
                        
                    } catch (const std::exception& err) {
                        // Ignore Errors
                    }
                    
                }
                // Performing the second scan on the found locations
                std::vector<size_t> newLocations;
                for (auto addr : locations) {
                    try {
                        auto memory = proc.readMemory(addr, targetBytes.size());
                        if (std::equal(targetBytes.begin(), targetBytes.end(), memory.begin())) {
                            newLocations.push_back(addr);
                        }
                    } catch (...) {
                        // Ignore errors for now
                    }
                }
                locations = std::move(newLocations);
                if ( locations.size() != 0){
                std::cout << "Name: "<< name <<  " Pid: "<< pid << ", Locations of exact match = "<< locations.size() << std::endl;
                }

                if ( pid == 26156){
                    int32_t newValue = 1000;
                    std::vector<uint8_t> newValueBytes = proc.to_ne_bytes(newValue);
    
                    for (const auto& addr : locations) {
                        try {
                            size_t bytesWritten = proc.writeMemory(proc.getHandle(), addr, newValueBytes);
                            std::cout << "Written " << bytesWritten << " bytes to [" << std::hex << addr << "]\n";
                        } catch (const std::exception& e) {
                            std::cerr << "Failed to write to [" << std::hex << addr << "]: " << e.what() << "\n";
                        }
                    }
                }

            } else {
                ++failed;
            }
        } else {
            ++failed;
        }
    }    
    return 0;
}