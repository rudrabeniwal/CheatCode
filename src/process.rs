use std::io;
use std::mem::MaybeUninit;
use std::ptr::NonNull;
use winapi::shared::minwindef::HMODULE;
use winapi::um::handleapi::CloseHandle;
use winapi::um::processthreadsapi::OpenProcess;
use winapi::um::psapi::{EnumProcessModules, GetModuleBaseNameA};
use winapi::um::winnt::{PROCESS_QUERY_INFORMATION, PROCESS_VM_READ};
use winapi::ctypes::c_void;

pub struct Process{
    pid: u32,
    handle: NonNull<c_void>,
}

impl Process {
    pub fn open(pid: u32) -> io::Result<Self> //self refers to the "Process" type itself
    {
        let handle = NonNull::new(unsafe{OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid)})
        
        .ok_or_else(io::Error::last_os_error)?;
        Ok(Self {pid, handle})
    }

    pub fn name(&self) -> io::Result<String> {
        let mut module = MaybeUninit::<HMODULE>::uninit();
        let mut size = 0;

        if unsafe {
            EnumProcessModules(self.handle.as_ptr(), module.as_mut_ptr(), std::mem::size_of::<HMODULE>() as u32, &mut size,)
        } == 0
        {
            return Err(io::Error::last_os_error());
        }

        let module = unsafe {
            module.assume_init()
        };
        self.get_module_base_name(module)
    }

    fn get_module_base_name (&self, module: HMODULE) -> io::Result<String> {
        let mut buffer = Vec::<u8>::with_capacity(64);

        let length = unsafe {
            GetModuleBaseNameA(self.handle.as_ptr(), module, buffer.as_mut_ptr().cast(), buffer.capacity() as u32,)
        };
        if length == 0{
            return Err(io::Error::last_os_error());
        }

        unsafe { buffer.set_len(length as usize)};
        Ok(String::from_utf8(buffer).unwrap())
    }
}
    
impl Drop for Process {
    fn drop(&mut self) {
        unsafe {CloseHandle(self.handle.as_mut())};
    }
}