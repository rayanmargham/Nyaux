#include "idt.h"
#include "drivers/serial.h"
#include "main.h"
#include "sched/sched.h"
#include "drivers/apic.h"

void (*idt_handlers[256])(struct StackFrame *frame);

struct IDTGateDescriptor IDT[256];
struct IDTR idtptr;
#define NYA_OS_IDT_PRESENT (1 << 7)
#define NYA_OS_IDT_RING0 (0 << 5)
#define NYA_OS_IDT_RING3 (0 << 5)
#define NYA_OS_IDT_TRAP 0xF
#define NYA_OS_IDT_INTERRUPT 0xE

extern void idt_flush(struct IDTR *idtptr);
void set_interrupt_descriptor(int vector, uint64_t handler, uint8_t selector, uint8_t type_attrib)
{
    IDT[vector].ist = 0;
    IDT[vector].type_attrib = type_attrib;
    IDT[vector].reversed = 0;
    IDT[vector].segment_sel = selector;
    IDT[vector].offset_low = (handler & 0xFFFF);
    IDT[vector].offset_mid = (handler >> 16) & 0xFFFF;
    IDT[vector].offset_hi = (handler >> 32) & 0xFFFFFFFF;
}
void RegisterHandler(int interrupt, void(*handler)(struct StackFrame *frame))
{
    idt_handlers[interrupt] = handler;
}

void division_by_zero(struct StackFrame *frame)
{
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}
void page_fault(struct StackFrame *frame)
{
    serial_print_color("Page Fault!\n", 4);
    char error[64] = "";
    serial_print_color(itoa(error, frame->error_code), 4);
    serial_print("\n");
    serial_print_color(itoah(error, frame->rip), 4);
    serial_print("\n");
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}

uint64_t sched_meow(struct StackFrame *frame)
{
    switch_task(frame);
    send_lapic_eoi();
    return (uint64_t)frame;
}
void init_idt()
{
    set_interrupt_descriptor(0, isr_stub_0, 0x28, NYA_OS_IDT_PRESENT | NYA_OS_IDT_RING0 | NYA_OS_IDT_INTERRUPT);
    set_interrupt_descriptor(0xE, isr_stub_14, 0x28, NYA_OS_IDT_PRESENT | NYA_OS_IDT_RING0 | NYA_OS_IDT_INTERRUPT);
    set_interrupt_descriptor(0x20, isr_stub_32, 0x28, NYA_OS_IDT_PRESENT | NYA_OS_IDT_RING0 | NYA_OS_IDT_INTERRUPT);
    RegisterHandler(0, division_by_zero);
    RegisterHandler(14, page_fault);
    RegisterHandler(32, sched_meow);
    idtptr.size = sizeof(IDT);
    idtptr.offset = &IDT;
    idt_flush(&idtptr);
}