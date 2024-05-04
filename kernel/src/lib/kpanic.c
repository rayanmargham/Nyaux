#include "kpanic.h"
#include <main.h>
#include <pmm.h>
#include <vmm.h>
#include <drivers/serial.h>
char c[64];

void kpanic(char *msg, struct StackFrame *frame)
{
    if (frame)
    {
        ctx->set_text_fg_rgb(ctx, 0xFFFFFFFF);
        write(ctx, "\e[44m\033[2J");
        ctx->clear(ctx, true);
        write(ctx, "Nya Kernel has Panicked with the Following Message: ");
        write(ctx, msg);
        write(ctx, "\n");
        write(ctx, "Register Dump: ");
        kprintf("RAX: 0x%lX RIP: 0x%lX\n", frame->rax, frame->rip);
        serial_print(itoah(c, frame->rip));
        serial_print("\n");
        asm ("cli"); // disable interrupts
        for (;;) {
        asm("hlt");
        }
    }
    else
    {
        ctx->set_text_fg_rgb(ctx, 0xFFFFFFFF);
        write(ctx, "\e[44m\033[2J");
        ctx->clear(ctx, true);
        write(ctx, "Nya Kernel has Panicked with the Following Message: ");
        write(ctx, msg);
        write(ctx, "\n");
        write(ctx, "No Register Dump Was Provided.");
        write(ctx, "\n");
        asm ("cli"); // disable interrupts
        for (;;) {
        asm("hlt");
        }
        
    }
}