#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>

class Process {
public:
    explicit Process(DWORD pid) 
        : pid_(pid), handle_(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) {
        if (handle_ == NULL) {
            std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        }
    }

    ~Process() {
        if (handle_ != NULL) {
            if (!CloseHandle(handle_)) {
                std::cerr << "Failed to close process handle. Error: " << GetLastError() << std::endl;
            }
        }
    }

    std::string name() const {
        HMODULE module;
        DWORD bytesReturned;
        if (!EnumProcessModules(handle_, &module, sizeof(module), &bytesReturned)) {
            std::cerr << "Failed to enumerate process modules. Error: " << GetLastError() << std::endl;
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
                std::cout << pid << ": " << name << std::endl;
                ++success;
            } else {
                ++failed;
            }
        } else {
            ++failed;
        }
    }

    std::cerr << "Successfully retrieved names for " << success << "/" << (success + failed) << " processes" << std::endl;
    return 0;
}