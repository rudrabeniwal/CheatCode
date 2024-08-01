#include <windows.h>
#include <iostream>
#include <vector>
#include <Psapi.h>

class Process {
public:
    // Constructor: attempts to open the process and store the handle.
    // explicit: Prevents implicit conversions, ensuring that the constructor is called only with an explicit DWORD argument.
    explicit Process(DWORD pid) : pid_(pid), handle_(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid)) // pid_(pid): Initializes the member variable pid_ with the given pid value.
   {
        if (handle_ == NULL) {
            // Handle the error if the process cannot be opened.
            std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        }
    }

    // Destructor: ensures the process handle is closed properly.
    ~Process() {
        if (handle_ != NULL) {
            if (!CloseHandle(handle_)) {
                // Handle the error if the handle cannot be closed.
                std::cerr << "Failed to close process handle. Error: " << GetLastError() << std::endl;
            }
        }
    }

    // Accessor method to get the handle for further operations.
    // HANDLE: The return type of the method.
    // getHandle(): The name of the method.
    // const: This keyword is used after the method declaration. It signifies that this method does not modify the state of the object.
    HANDLE getHandle() const { return handle_; }

private:
    DWORD pid_;    // Process ID of the target process.
    HANDLE handle_; // Handle to the process.
};

// Function to enumerate all processes and return their IDs.
std::vector<DWORD> enumProc() {
    std::vector<DWORD> pids(1024); // Allocate space for up to 1024 PIDs.
    DWORD bytesReturned;

    // Call EnumProcesses to populate the pids vector with process IDs.
    if (!EnumProcesses(pids.data(), pids.size() * sizeof(DWORD), &bytesReturned)) {
        // Handle the error if enumeration fails.
        std::cerr << "Failed to enumerate processes. Error: " << GetLastError() << std::endl;
        return {};
    }

    // Resize the vector to the number of PIDs returned.
    pids.resize(bytesReturned / sizeof(DWORD));
    return pids;
}

int main() {
    // Get the list of process IDs.
    auto pids = enumProc();
    int success = 0;
    int failed = 0;

    // Iterate over each PID and attempt to open the process.
    for (DWORD pid : pids) {
        Process proc(pid);
        if (proc.getHandle() != NULL) {
            // Increment success count if the process was opened successfully.
            ++success;
        } else {
            // Increment failed count if the process could not be opened.
            ++failed;
        }
    }

    // Output the results of process opening.
    std::cerr << "Successfully opened " << success << "/" << (success + failed) << " processes" << std::endl;
    return 0;
}