mod enum_proc;

use enum_proc::enum_proc;
fn main() {
    dbg!(enum_proc().unwrap().len());
}