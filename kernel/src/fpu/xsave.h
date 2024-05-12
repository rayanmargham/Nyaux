#pragma once
#include <stdint.h>
#include <stddef.h>
extern uint64_t get_xsave_region_size();
extern uint64_t xsave_size;
void setup_xsave();
static inline void xsave(uint64_t to_addr)
{
    register uint32_t edx asm("edx") = 0xFFFFFFFF;
    register uint32_t eax asm("eax") = 0xFFFFFFFF;
    asm volatile("xsaveq (%0)" : : "r"(to_addr), "r"(edx), "r"(eax) : "memory");
}
static inline void xrstore(uint64_t from_addr)
{
    register uint32_t edx asm("edx") = 0xFFFFFFFF;
    register uint32_t eax asm("eax") = 0xFFFFFFFF;
    asm volatile("xrstorq (%0)" : : "r"(from_addr), "r"(edx), "r"(eax) : "memory");
}