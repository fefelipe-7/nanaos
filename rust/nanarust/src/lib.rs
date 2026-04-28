#![no_std]

use core::ffi::c_char;

unsafe extern "C" {
    fn terminal_write(data: *const c_char);
    fn serial_write(data: *const c_char);
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

#[unsafe(no_mangle)]
pub extern "C" fn nanarust_init() {
    static MSG: &[u8] = b"[OK] nanarust initialized\n\0";
    unsafe {
        terminal_write(MSG.as_ptr() as *const c_char);
        serial_write(MSG.as_ptr() as *const c_char);
    }
}
