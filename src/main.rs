mod ui;
use std::fmt;
use std::io;
use std::mem::{self, MaybeUninit};
use std::ptr::NonNull;
use winapi::ctypes::c_void;
use winapi::shared::minwindef::{DWORD, FALSE, HMODULE};
use winapi::um::winnt;

const MAX_PIDS: usize = 1024;

const MAX_PROC_NAME_LEN: usize = 64;

#[derive(Debug)]
pub struct Process {
    pid: u32,
    handle: NonNull<c_void>,
}

pub fn enum_proc() -> io::Result<Vec<u32>> {
    let mut size = 0;
    let mut pids = Vec::<DWORD>::with_capacity(MAX_PIDS);

    if unsafe {
        winapi::um::psapi::EnumProcesses(pids.as_mut_ptr(), (pids.capacity() * mem::size_of::<DWORD>()) as u32, &mut size,)
    } ==FALSE {
        return Err(io::Error::last_os_error());
    }

    let count = size as usize / mem::size_of::<DWORD>();
    unsafe {pids.set_len(count)};
    Ok(pids)
}

impl Process {
    pub fn pid(&self) -> u32{
        self.pid
    }
    pub fn name(&self) ->io::Result<String>{
        let mut module = MaybeUninit::<HMODULE>::uninit();
        let mut size = 0;

        if unsafe {
            winapi::um::psapi::EnumProcessModules(self.handle.as_ptr(), module.as_mut_ptr(), mem::size_of::<HMODULE>() as u32, &mut size)
        } == FALSE {
            return Err(io::Error::last_os_error());
        }

        let module = unsafe {module.assume_init()};

        let mut buffer = Vec::<u8>::with_capacity(MAX_PROC_NAME_LEN);

        let length = unsafe {
            winapi::um::psapi::GetModuleBaseNameA(self.handle.as_ptr(), module, buffer.as_mut_ptr().cast(), buffer.capacity() as u32,)
        };
        if length == 0 {
            return Err(io::Error::last_os_error());
        }

        unsafe{ buffer.set_len(length as usize)};
        Ok(String::from_utf8(buffer).unwrap())
    }

    pub fn open(pid: u32) -> io::Result<Self> {
        NonNull::new(unsafe {
            winapi::um::processthreadsapi::OpenProcess(
                winnt::PROCESS_QUERY_INFORMATION | winnt::PROCESS_VM_READ,
                FALSE,
                pid,
            )
        })
        .map(|handle| Self { pid, handle })
        .ok_or_else(io::Error::last_os_error)
    }

    pub fn memory_regions(&self) -> Vec<winapi::um::winnt::MEMORY_BASIC_INFORMATION> {
        let mut base = 0;
        let mut regions = Vec::new();
        let mut info = MaybeUninit::uninit();

        loop {
            let written = unsafe {
                winapi::um::memoryapi::VirtualQueryEx(self.handle.as_ptr(), base as *const _, info.as_mut_ptr(), mem::size_of::<winapi::um::winnt::MEMORY_BASIC_INFORMATION>())
            };
            if written == 0{
                break regions;
            }
            let info = unsafe { info.assume_init() };
            base = info.BaseAddress as usize + info.RegionSize;
            regions.push(info);
        }
    }

    pub fn read_memory(&self, addr: usize, n: usize) -> io::Result<Vec<u8>> {
        let mut buffer = Vec::<u8>::with_capacity(n);
        let mut read = 0;
    
        if unsafe {
            winapi::um::memoryapi::ReadProcessMemory(self.handle.as_ptr(), addr as *const _, buffer.as_mut_ptr().cast(), buffer.capacity(), &mut read)
        } == FALSE {
            Err(io::Error::last_os_error())
        } else {
            unsafe {buffer.set_len(read as usize)};
            Ok(buffer)
        }
    }
}

impl Drop for Process {
    fn drop(&mut self) {
        let ret = unsafe { winapi::um::handleapi::CloseHandle(self.handle.as_mut())};
        assert_ne!(ret, FALSE);
    }
}

struct ProcessItem {
    pid: u32,
    name: String
}

impl fmt::Display for ProcessItem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} (pid={})", self.name, self.pid)
    }
}

fn main() {
    let processes = enum_proc()
        .unwrap()//unwrap() is used to extract the Ok value from the Result
        .into_iter()//Converts the Vec<u32> of PIDs into an iterator
        .flat_map(Process::open)
        .flat_map(|proc| match proc.name() {
            Ok(name) => Ok(ProcessItem {
                pid: proc.pid(),
                name,
            }),
            Err(err) => Err(err),
        })
        .collect::<Vec<_>>();
    let item = ui::list_picker(&processes);
    let process = Process::open(item.pid).unwrap();
    println!("Opened process {:?}", process);

    dbg!(process.memory_regions().len());
}