use std::io;
use std::ptr::NonNull;
use winapi::ctypes::c_void;
use winapi::um::processthreadsapi::OpenProcess;
use winapi::um::handleapi::CloseHandle;
use winapi::um::winnt::PROCESS_QUERY_INFORMATION;
use winapi::shared::minwindef::FALSE;

pub struct Process{
    pid: u32,
    handle: NonNull<c_void>,
}

impl Process {
    pub fn open(pid: u32) -> io::Result<Self> //self refers to the "Process" type itself
    {
        let handle = unsafe {
            NonNull::new(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid))
        }
        .ok_or_else(io::Error::last_os_error)?;

    Ok(Self {pid, handle})
    }
    
    pub fn get_pid(&self) -> u32 {
        self.pid
    }
}
    
impl Drop for Process {
    fn drop(&mut self) {
        unsafe {CloseHandle(self.handle.as_mut())};
    }
}