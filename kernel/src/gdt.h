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