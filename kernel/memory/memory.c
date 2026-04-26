#include "memory.h"
#include "terminal/vga.h"
#include "lib/string.h"
#include "memory/pmm.h"
#include <stdint.h>

void print_meminfo(void) {
    uint64_t total = pmm_total_memory();
    uint64_t free = pmm_free_memory();
    uint64_t used = pmm_used_memory();

    char buf[32];
    itoa(total / 1024, buf, 10);
    terminal_write_string("Total Memory: "); terminal_write_string(buf); terminal_write_string(" KB\n");

    itoa(free / 1024, buf, 10);
    terminal_write_string("Free Memory : "); terminal_write_string(buf); terminal_write_string(" KB\n");

    itoa(used / 1024, buf, 10);
    terminal_write_string("Used Memory : "); terminal_write_string(buf); terminal_write_string(" KB\n");

    /* Approx frame counts */
    uint64_t free_frames = free / 4096;
    uint64_t used_frames = used / 4096;
    itoa(free_frames, buf, 10);
    terminal_write_string("Free frames : "); terminal_write_string(buf); terminal_write_string("\n");
    itoa(used_frames, buf, 10);
    terminal_write_string("Used frames : "); terminal_write_string(buf); terminal_write_string("\n");
}

void print_uptime(void) {
    /* Timer/PIT provides uptime_seconds() */
    extern uint64_t uptime_seconds(void);
    uint64_t s = uptime_seconds();
    char buf[32];
    itoa(s, buf, 10);
    terminal_write_string("Uptime: ");
    terminal_write_string(buf);
    terminal_write_string(" segundos\n");
}
