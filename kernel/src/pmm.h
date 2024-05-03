#pragma once
#include <stdint.h>
#include <stddef.h>

void pmm_init();

struct pmm_node
{
    struct pmm_node *next;
};
struct Header
{
    size_t size;
    struct Header *next;
    struct pmm_node *freelist;
};
struct cache
{
    struct Header *slabs;
    size_t size;
};
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_memmap_request memmap_request;
void *pmm_alloc_singlep();
void pmm_free_singlep(void *page);
// void *slab_alloc(size_t size);
// void slab_free(void *addr);
void free(void *addr);
void *kmalloc(size_t size);
void kfree(void *addr, size_t size);
void *krealloc(void *addr, size_t oldsz, size_t newsz);