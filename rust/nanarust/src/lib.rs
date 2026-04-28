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

fn cstr_len(mut p: *const u8) -> usize {
    let mut n = 0usize;
    unsafe {
        while *p != 0 {
            n += 1;
            p = p.add(1);
        }
    }
    n
}

fn to_upper_ascii(b: u8) -> u8 {
    if (b'a'..=b'z').contains(&b) {
        b - (b'a' - b'A')
    } else {
        b
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn nanarust_cmd_eq(a: *const c_char, b: *const c_char) -> i32 {
    if a.is_null() || b.is_null() {
        return 0;
    }
    let la = cstr_len(a as *const u8);
    let lb = cstr_len(b as *const u8);
    if la != lb {
        return 0;
    }
    unsafe {
        for i in 0..la {
            let ca = to_upper_ascii(*(a as *const u8).add(i));
            let cb = to_upper_ascii(*(b as *const u8).add(i));
            if ca != cb {
                return 0;
            }
        }
    }
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn nanarust_init() {
    static MSG: &[u8] = b"[OK] nanarust initialized\n\0";
    unsafe {
        terminal_write(MSG.as_ptr() as *const c_char);
        serial_write(MSG.as_ptr() as *const c_char);
    }
}
