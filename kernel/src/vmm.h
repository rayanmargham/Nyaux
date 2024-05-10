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

void update_cr3(uint64_t cr3_value);
#define NYA_OS_VMM_PRESENT 1
#define NYA_OS_VMM_RW (1 << 1)
#define NYA_OS_VMM_USER (1 << 2)
void map(uint64_t *pml4, uint64_t virt, uint64_t phys, uint8_t flags);
uint64_t *read_cr3();
uint64_t *read_cr4();
struct pagemap // represents a pagemap.
{
    struct vmm_region *head;
    uint64_t *pml4;
};
void update_cr4(uint64_t cr4_value);
extern struct pagemap kernel_pagemap;
struct pagemap *new_pagemap();
void vmm_region_dealloc_user(struct pagemap *user_map, uint64_t base);
void *vmm_region_alloc_user(struct pagemap *user_map, uint64_t size, uint8_t flags);
uint64_t virt_to_phys(uint64_t *pml4, uint64_t virt);