#pragma once
#include <stdint.h>
#include <stddef.h>

struct StackFrame
{
    uint64_t intnum, r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rbp, error_code, rip, cs, rflags, rsp, ss;
} __attribute__ ((packed));

struct IDTGateDescriptor
{
    uint16_t offset_low;
    uint16_t segment_sel;
    uint8_t ist;
    uint8_t type_attrib;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t reversed;
} __attribute__ ((packed));

struct IDTR
{
    uint16_t size;
    uint64_t offset;
} __attribute__ ((packed));

extern void isr_stub_0();
extern void isr_stub_14();
extern void isr_stub_32();
void init_idt();