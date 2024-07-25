mod enum_proc;
mod process;

use enum_proc::enum_proc;
use process::Process;

fn main() {
    let mut success = 0;
    let mut failed = 0;

    for pid in enum_proc().unwrap() /*The unwrap() call is used to get the result from enum_proc(), assuming it’s Ok. If enum_proc() returns an Err, the program will panic.*/ {
        match Process::open(pid) {
            Ok(process) => { 
                println!("Opened process with PID: {}", process.get_pid());
                success += 1;
            },
            Err(_) => failed += 1,
        }
    }

    eprintln!("Successfully opened {}/{} processes", success, success + failed);
}
