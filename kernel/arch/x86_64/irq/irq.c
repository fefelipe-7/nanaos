#include "arch/x86_64/irq/irq.h"
#include "arch/x86_64/exceptions/exceptions.h"
#include "arch/x86_64/pic/pic.h"
#include "drivers/timer/pit.h"
#include "drivers/keyboard/keyboard.h"
#include "syscall/syscall.h"
#include <stdint.h>

/* C-level dispatcher: called from the common assembly stub. */
void isr_common_c(uint64_t vector, uint64_t error_code) {
    if (vector >= 32 && vector < 48) {
        uint8_t irq = (uint8_t)(vector - 32);

        /* Call small per-IRQ handlers (timer, keyboard); keep them fast. */
        switch (irq) {
            case 0:
                pit_on_interrupt();
                break;
            case 1:
                keyboard_on_interrupt();
                break;
            default:
                /* unhandled IRQ — nothing for now */
                break;
        }

        /* Acknowledge PICs */
        pic_send_eoi(irq);
        return;
    }

    /* Syscall via int 0x80 (vector 128) */
    if (vector == 128) {
        /* Read pointer to saved registers (we pushed registers in assembly)
         * stack[0] = RAX, [1]=RBX, [2]=RCX, [3]=RDX, [4]=RSI, [5]=RDI,
         * [7]=R8, [8]=R9
         */
        uint64_t *stackptr;
        asm volatile("mov %%rsp, %0" : "=r"(stackptr));

        uint64_t syscall_nr = stackptr[0]; /* RAX */
        uint64_t a1 = stackptr[5]; /* RDI */
        uint64_t a2 = stackptr[4]; /* RSI */
        uint64_t a3 = stackptr[3]; /* RDX */
        uint64_t a4 = stackptr[2]; /* RCX */
        uint64_t a5 = stackptr[7]; /* R8  */
        uint64_t a6 = stackptr[8]; /* R9  */

        uint64_t ret = syscall_dispatch(syscall_nr, a1, a2, a3, a4, a5, a6);

        /* write return value back into saved RAX so it is restored to user */
        stackptr[0] = ret;
        return;
    }

    /* Not an IRQ we handle here — forward to exception handler */
    exception_dispatch(vector, error_code);
}
