#pragma once
#include <stdint.h>

struct GDTDescriptor
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access_byte;
    uint8_t flag_and_hi_limit;
    uint8_t base_hi;
} __attribute__((packed));

struct GDTR
{
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));
void init_gdt();
struct TSS
{
    uint32_t rev3;
    uint64_t rsp0;
    uint64_t rsp1;
    uint32_t rev2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t re2;
    uint16_t rev1;
    uint16_t iopb;
} __attribute__((packed));
extern struct TSS ts;