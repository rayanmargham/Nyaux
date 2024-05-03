#pragma once
#include <stdint.h>
#include <stddef.h>

void vmm_init();
uint64_t align_down(uint64_t addr, size_t page_size);
uint64_t align_up(uint64_t addr, size_t page_size);
struct vmm_region
{
    uint64_t base;
    uint64_t length;
    uint8_t flags;
    struct vmm_region *next;
};
void vmm_region_dealloc(uint64_t base);
void *vmm_region_alloc(uint64_t size, uint8_t flags);
extern uint64_t *pml4;
void update_cr3(uint64_t cr3_value);
#define NYA_OS_VMM_PRESENT 1
#define NYA_OS_VMM_RW (1 << 1)
uint64_t *read_cr3();