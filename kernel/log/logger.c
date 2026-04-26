#include "logger.h"
#include "drivers/serial/com1.h"
#include "terminal/vga.h"

void log_info(const char *message) {
    serial_write_string("[INFO] ");
    serial_write_string(message);
    serial_write_string("\n");
    terminal_writeln(message);
}

void log_error(const char *message) {
    serial_write_string("[ERROR] ");
    serial_write_string(message);
    serial_write_string("\n");
    terminal_writeln(message);
}

void log_warn(const char *message) {
    serial_write_string("[WARN] ");
    serial_write_string(message);
    serial_write_string("\n");
    terminal_writeln(message);
}
