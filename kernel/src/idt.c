#include "idt.h"
#include "drivers/serial.h"
#include "main.h"
#include "sched/sched.h"
#include "drivers/apic.h"
#include <lib/kpanic.h>

void (*idt_handlers[256])(struct StackFrame *frame);

struct IDTGateDescriptor IDT[256];
struct IDTR idtptr;
extern uint64_t stubs[];
int cur_vec_free = 32;
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
int AllocateVector()
{
    return cur_vec_free++;
}
void division_by_zero(struct StackFrame *frame)
{
    kpanic("DIVISON BY ZERO", frame);
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}
void page_fault(struct StackFrame *frame)
{
    kpanic("PAGE FAULT VIOLATION", frame);
}

uint64_t sched_meow(struct StackFrame *frame)
{
    switch_task(frame);
    send_lapic_eoi();
    return (uint64_t)frame;
}
void init_idt()
{
    for (int i = 0; i < 256; i++)
    {
        set_interrupt_descriptor(i, stubs[i], 0x28, NYA_OS_IDT_INTERRUPT | NYA_OS_IDT_PRESENT | NYA_OS_IDT_RING3);
    }
    RegisterHandler(0, division_by_zero);
    RegisterHandler(14, page_fault);
    RegisterHandler(AllocateVector(), sched_meow);
    idtptr.size = sizeof(IDT);
    idtptr.offset = &IDT;
    idt_flush(&idtptr);
}