use std::io;
use std::mem;
use winapi::shared::minwindef::{DWORD, FALSE};
use winapi::um::psapi::EnumProcesses;

pub fn enum_proc() -> io::Result<Vec<u32>> {
    let mut pids = Vec::<DWORD>::with_capacity(1024);
    let mut size = 0;

    if unsafe {
        EnumProcesses(
            pids.as_mut_ptr(), //A mutable pointer to the vector's buffer. This is where the process IDs will be stored.
            (pids.capacity() * mem::size_of::<DWORD>()) as u32, //Total size in bytes
            &mut size, 
        )
    } == FALSE
    {
        return Err(io::Error::last_os_error());
    }

    let count = size as usize / mem::size_of::<DWORD>(); //get number of process id's
    unsafe {pids.set_len(count)};
    Ok(pids)
}
