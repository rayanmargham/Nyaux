#include "kpanic.h"
#include <main.h>
#include <pmm.h>
#include <vmm.h>
#include <drivers/serial.h>
char c[64];
uint64_t read_cr2()
{
    uint64_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2) :: "memory");
    return cr2;
}
void kpanic(char *msg, struct StackFrame *frame)
{
    if (frame)
    {
        ctx->set_text_fg_rgb(ctx, 0xFFFFFFFF);
        //write(ctx, "\e[41m\033[2J");
        //ctx->clear(ctx, true);
        write(ctx, "Nya Kernel has Panicked with the Following Message: ");
        write(ctx, msg);
        write(ctx, "\n");
        write(ctx, "Register Dump: ");
        kprintf("RAX: 0x%lX RIP: 0x%lX\n", frame->rax, frame->rip);
        serial_print("RIP: ");
        serial_print(itoah(c, frame->rip));
        serial_print("\n");
        serial_print("CR2: ");
        serial_print(itoah(c, read_cr2()));
        serial_print("\n");
        kprintf("CR2: 0x%lX\n", read_cr2());
        kprintf("\nSTACKTRACE: \n");
        
        // STACK TRACE TIME :sunglasses:
        uint64_t *bas = frame->rbp;
        kprintf("--> 0x%lX\n", frame->rip);
        while (bas != NULL)
        {
            uint64_t ret_addr = *(uint64_t*)((uint64_t)bas + sizeof(uint64_t)); // CAUSE TRACK GO DOWNWARD LOL
            if (ret_addr == 0)
            {
                break;
            }
            kprintf("^-> 0x%lX in\n", ret_addr - 1); // FAULTING INSTRUCTION
            bas = *bas;
        }
        asm ("cli"); // disable interrupts
        for (;;) {
        asm("hlt");
        }
    }
    else
    {
        ctx->set_text_fg_rgb(ctx, 0xFFFFFFFF);
        //write(ctx, "\e[41m\033[2J");
        //ctx->clear(ctx, true);
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