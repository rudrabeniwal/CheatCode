#include <iostream>
#include "process.h"
#include "ui.h"

void listProcesses() {
    auto pids = enumProc();
    for (DWORD pid : pids) {
        Process proc(pid);
        if (proc.getHandle() != NULL) {
            std::string name = proc.name();
            if (!name.empty()) {
                std::cout << "PID: " << pid << ", Name: " << name << std::endl;
            }
        }
    }
}

void runUI() {
    std::cout << "Available Processes:\n";
    listProcesses();

    DWORD pid;
    std::cout << "\nEnter PID of the process to scan: ";
    std::cin >> pid;

    char scanType;
    std::cout << "Scan for exact value or range? (e/r): ";
    std::cin >> scanType;

    int32_t targetValue;
    int32_t minValue, maxValue;

    if (scanType == 'e') {
        std::cout << "Enter the target value to scan for: ";
        std::cin >> targetValue;
    } else if (scanType == 'r') {
        std::cout << "Enter the minimum value: ";
        std::cin >> minValue;
        std::cout << "Enter the maximum value: ";
        std::cin >> maxValue;
    } else {
        std::cerr << "Invalid input. Exiting...\n";
        return;
    }

    Process proc(pid);
    if (proc.getHandle() == NULL) {
        std::cerr << "Failed to open process with PID: " << pid << std::endl;
        return;
    }

    std::vector<size_t> locations;

    if (scanType == 'e') {
        locations = exactValueScan(proc, targetValue);
    } else if (scanType == 'r') {
        locations = rangeValueScan(proc, minValue, maxValue);
    }

    if (!locations.empty()) {
        std::cout << "Found " << locations.size() << " locations with the specified value(s).\n";
        for (size_t loc : locations) {
            std::cout << "Address: " << std::hex << loc << std::endl;
        }

        int32_t newValue;
        std::cout << "\nEnter new value to write: ";
        std::cin >> newValue;

        std::vector<uint8_t> newValueBytes = proc.to_ne_bytes(newValue);

        for (const auto& addr : locations) {
            try {
                size_t bytesWritten = proc.writeMemory(proc.getHandle(), addr, newValueBytes);
                std::cout << "Written " << bytesWritten << " bytes to [" << std::hex << addr << "]\n";
            } catch (const std::exception& e) {
                std::cerr << "Failed to write to [" << std::hex << addr << "]: " << e.what() << "\n";
            }
        }
    } else {
        std::cout << "No locations found with the specified value(s).\n";
    }
}
