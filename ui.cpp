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

    Process proc(pid);

    std::vector<MemoryRegion> potentialRegions;

    int scanType;
    std::cout << "Select scan type:\n";
    std::cout << "1. Exact Value Scan\n";
    std::cout << "2. Range Value Scan\n";
    std::cout << "Enter choice: ";
    std::cin >> scanType;

    if (scanType == 1) {
        int32_t target;
        std::cout << "Enter the exact value to scan for: ";
        std::cin >> target;
        potentialRegions = exactValueScan(proc, target);
    } else if (scanType == 2) {
        int32_t minValue, maxValue;
        std::cout << "Enter the minimum value: ";
        std::cin >> minValue;
        std::cout << "Enter the maximum value: ";
        std::cin >> maxValue;
        potentialRegions = rangeValueScan(proc, minValue, maxValue);
    } else {
        std::cerr << "Invalid choice\n";
        return;
    }

    if (potentialRegions.empty()) {
            std::cout << "No matching regions found.\n";
        } else {
            std::cout << std::endl;
            std::cout << "!! Found " << potentialRegions.size() << " matching regions !!\n";
    }

    int followUpScanType;
    while (true) {
        std::cout << std::endl;
        std::cout << "Select follow-up scan type:\n";
        std::cout << "1. Increased Value\n";
        std::cout << "2. Increased Value By...\n";
        std::cout << "3. Decreased Value\n";
        std::cout << "4. Decreased Value By...\n";
        std::cout << "5. Changed Value\n";
        std::cout << "6. Unchanged Value\n";
        std::cout << "=> 7. Proceed to the Next Step =>\n";
        std::cout << "8. Exit X|\n";
        std::cout << "Enter choice: ";
        std::cin >> followUpScanType;

        switch (followUpScanType) {
            case 1:
                potentialRegions = increasedValueScan(proc, potentialRegions);
                break;
            case 2: {
                int32_t amount;
                std::cout << "Enter the amount: ";
                std::cin >> amount;
                potentialRegions = increasedValueByScan(proc, potentialRegions, amount);
                break;
            }
            case 3:
                potentialRegions = decreasedValueScan(proc, potentialRegions);
                break;
            case 4: {
                int32_t amount;
                std::cout << "Enter the amount: ";
                std::cin >> amount;
                potentialRegions = decreasedValueByScan(proc, potentialRegions, amount);
                break;
            }
            case 5:
                potentialRegions = changedValueScan(proc, potentialRegions);
                break;
            case 6:
                potentialRegions = unchangedValueScan(proc, potentialRegions);
                break;
            case 7: {
                if (potentialRegions.empty()) {
                    std::cout << "No regions available for writing.\n";
                    break;
                }

                int32_t newValue;
                std::cout << "Enter the new value to write: ";
                std::cin >> newValue;

                auto newValueBytes = proc.to_ne_bytes(newValue);

                for (const auto& region : potentialRegions) {
                    try {
                        size_t written = proc.writeMemory(proc.getHandle(), region.address, newValueBytes);
                        std::cout << "Wrote " << written << " bytes to address 0x" << std::hex << region.address << std::dec << ".\n";
                    } catch (const std::exception& ex) {
                        std::cerr << "Failed to write to address 0x" << std::hex << region.address << ": " << ex.what() << std::dec << "\n";
                    }
                }

                break;
            }
            case 8:
                return;
            default:
                std::cerr << "Invalid choice\n";
                continue;
        }

        if (potentialRegions.empty()) {
            std::cout << "No matching regions found.\n";
            break;
        } else {
            std::cout << std::endl;
            std::cout << "!! Found " << potentialRegions.size() << " matching regions !!\n";
        }
    }
}