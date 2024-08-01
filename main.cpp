#include <windows.h>
#include <psapi.h>
#include <vector>
#include <iostream>

std::vector<DWORD> enumProc() {
    std::vector<DWORD> pids(1024); // Allocate space for 1024 PIDs
    DWORD bytesReturned;

    if (!EnumProcesses(pids.data(), pids.size() * sizeof(DWORD), &bytesReturned)) {
        // Error handling: call GetLastError for more info
        std::cerr << "Failed to enumerate processes. Error: " << GetLastError() << std::endl;
        return {};
    }

    // Calculate number of processes returned
    pids.resize(bytesReturned / sizeof(DWORD));
    return pids;
}

int main() {
    auto pids = enumProc();
    std::cout << "Number of processes: " << pids.size() << std::endl;
    return 0;
}
