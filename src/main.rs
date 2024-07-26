mod enum_proc;
mod process;

use enum_proc::enum_proc;
use process::Process;

fn main() {
    let mut success = 0;
    let mut failed = 0;

    for pid in enum_proc().unwrap() {
        match Process::open(pid) {
            Ok(proc) => match proc.name() {
                Ok(name) => {
                    println!("{}: {}", pid, name);
                    success += 1;
                }
                Err(e) => {
                    println!("{}: (failed to get name: {})", pid, e);
                    failed += 1;
                }
            },
            Err(e) => {
                println!("Failed to open {}: {}", pid, e);
                failed += 1;
            }
        }
    }

    eprintln!("Successfully opened {}/{} processes", success, success + failed);
}
