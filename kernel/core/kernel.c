#include "kernel.h"
#include "drivers/serial/com1.h"
#include "terminal/vga.h"
#include "panic/panic.h"
#include "log/logger.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/pic/pic.h"
#include "drivers/timer/pit.h"
#include "drivers/keyboard/keyboard.h"
#include "core/shell.h"
#include "fs/vfs.h"
/* Disk and FAT support */
#include "drivers/ata/ata.h"
#include "drivers/block/block.h"
#include "fs/mbr.h"
#include "fs/fat32.h"
#include "memory/vmm.h"
#include "drivers/framebuffer/fb.h"
#include "drivers/mouse/mouse.h"
#include "core/events.h"
#include "ui/nanaura/ura.h"
#include "ui/system/ui.h"

void nanarust_init(void);

void kernel_main(void *multiboot_info) {
    terminal_initialize();
    terminal_clear();
    terminal_writeln("nanaos booting...");

    serial_init();
    log_info("[OK] serial ready");
    log_info("[OK] terminal ready");
    log_info("[OK] nanacore initialized");
    events_init();

    /* Initialize physical memory manager using Multiboot2 info */
    extern void pmm_init(uint64_t multiboot_addr);
    pmm_init((uint64_t)multiboot_info);
    log_info("[OK] memory ready");
    vmm_init();

    gdt_init();
    log_info("[OK] gdt loaded");

    idt_init();
    log_info("[OK] idt loaded");
    log_info("[OK] exceptions ready");

    /* Initialize PIC, timer and keyboard (IRQs) */
    pic_init();
    log_info("[OK] pic ready");

    pit_init(100); /* 100 Hz */
    log_info("[OK] timer ready");

    keyboard_init();
    log_info("[OK] keyboard ready");
    mouse_init();

    int graphics = (fb_init((uint64_t)multiboot_info) == 0);
    if (graphics) {
        fb_clear(0x00102030);
    }

    /* Initialize virtual filesystem and shell */
    vfs_init();
    log_info("[OK] vfs ready");

    /* Attempt to initialize ATA and mount first FAT32 partition at /disk */
    if (ata_init() == 0) {
        log_info("[OK] disk ready");
        ata_selftest_sector0();
        block_device_t *b = block_get_primary();
        uint64_t part_lba = 0, part_sectors = 0;
        if (mbr_find_fat32_partition(b, &part_lba, &part_sectors) == 0) {
            if (fat32_mount_partition(b, part_lba, "/disk") == 0) {
                log_info("[OK] fat32 mounted");
            } else {
                log_warn("FAT32 mount failed");
            }
        } else {
            log_warn("No FAT32 partition found");
        }
    } else {
        log_warn("ATA init failed");
    }

    asm volatile("sti");

    /* Scheduler: initialize but do not start automatic preemption yet */
    extern void scheduler_init(void);
    scheduler_init();
    log_info("[OK] scheduler ready");
    nanarust_init();

    /* Optional: create demo processes (comment out if not desired) */
    /*
    extern process_t *process_create(const char*, void(*)(void*), void*, int);
    void demo_proc1(void *arg) {
        (void)arg;
        while (1) {
            terminal_write_string("[proc1] running\n");
            for (volatile int i = 0; i < 1000000; ++i) ;
            extern void scheduler_yield(void);
            scheduler_yield();
        }
    }
    process_create("proc1", demo_proc1, NULL, 1);
    */

    if (graphics) {
        if (ui_system_init(multiboot_info) == 0) {
            ui_system_run();
        }
    }

    /* fallback: shell no modo texto */
    shell_init();
    for (;;) asm volatile("hlt");
}
