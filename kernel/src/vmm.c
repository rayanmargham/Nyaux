#include "pmm.h"
#include "drivers/serial.h"
#include "main.h"
#include "vmm.h"

extern char THE_REAL[];
#define NYA_OS_VMM_PRESENT 1
#define NYA_OS_VMM_RW (1 << 1)
#define NYA_OS_VMM_USER (1 << 2)
struct vmm_region *vmm_head;
struct limine_kernel_address_request addr_request = 
{
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 1
};
void update_cr3(uint64_t cr3_value)
{
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint64_t)cr3_value) : "memory");
}
uint64_t *read_cr3()
{
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3) :: "memory");
    return cr3;
}
uint64_t align_down(uint64_t addr, size_t page_size)
{
    return addr & ~(page_size - 1);
}
uint64_t align_up(uint64_t addr, size_t page_size)
{
    return (addr + (page_size - 1)) & ~(page_size - 1);
}

uint64_t *pml4;
uint64_t *get_next_table(uint64_t *table, uint64_t index, uint8_t flags)
{
    if ((table[index] & NYA_OS_VMM_PRESENT) == 0)
    {
        uint64_t *new_table = pmm_alloc_singlep();
        memset((uint64_t)new_table + hhdm_request.response->offset, 0, 4096);
        uint64_t entry = 0;
        entry = (uint64_t)new_table;
        entry = entry | flags;
        table[index] = entry;
        return (table[index] & 0x000FFFFFFFFFF000);
    }
    else
    {
        return (table[index] & 0x000FFFFFFFFFF000);
    }
}

void map(uint64_t *pml4, uint64_t virt, uint64_t phys, uint8_t flags)
{
    uint64_t lvl4_index = (virt >> 39) & 0x1FF;
    uint64_t lvl3_index = (virt >> 30) & 0x1FF;
    uint64_t lvl2_index = (virt >> 21) & 0x1FF;
    uint64_t lvl1_index = (virt >> 12) & 0x1FF;

    uint64_t *cur_table = pml4;
    cur_table = get_next_table(pml4, lvl4_index, flags);
    cur_table = get_next_table((uint64_t)cur_table + hhdm_request.response->offset, lvl3_index, flags);
    cur_table = get_next_table((uint64_t)cur_table + hhdm_request.response->offset, lvl2_index, flags);

    uint64_t entry = 0;
    entry = phys;
    entry = entry | flags;
    ((uint64_t*)((uint64_t)cur_table + hhdm_request.response->offset))[lvl1_index] = entry;
}
uint64_t *get_next_table_unmap(uint64_t *table, uint64_t index)
{
    if (table != NULL)
    {
        if ((table[index] & NYA_OS_VMM_PRESENT) == 1)
        {
            // EXISTS
            return (table[index] & 0x000FFFFFFFFFF000);
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}
uint64_t unmap(uint64_t *pml4, uint64_t virt)
{
    uint64_t old = 0;
    uint64_t lvl4_index = (virt >> 39) & 0x1FF;
    uint64_t lvl3_index = (virt >> 30) & 0x1FF;
    uint64_t lvl2_index = (virt >> 21) & 0x1FF;
    uint64_t lvl1_index = (virt >> 12) & 0x1FF;

    uint64_t *cur_table = pml4;
    cur_table = get_next_table_unmap(pml4, lvl4_index);
    if (cur_table == NULL)
    {
        return 0;
    }
    cur_table = get_next_table_unmap((uint64_t)cur_table + hhdm_request.response->offset, lvl3_index);
    if (cur_table == NULL)
    {
        return 0;
    }
    cur_table = get_next_table_unmap((uint64_t)cur_table + hhdm_request.response->offset, lvl2_index);

    if (cur_table != NULL)
    {
        if (((uint64_t*)((uint64_t)cur_table + hhdm_request.response->offset))[lvl1_index] != NULL)
        {
            old = ((uint64_t*)((uint64_t)cur_table + hhdm_request.response->offset))[lvl1_index];
        }
        ((uint64_t*)((uint64_t)cur_table + hhdm_request.response->offset))[lvl1_index] = 0;
        __asm__ volatile ("invlpg (%0)" :: "b" (virt) : "memory");
    }
    return old;

}

void *vmm_region_alloc(uint64_t size, uint8_t flags)
{
    struct vmm_region *cur_node = vmm_head;
    struct vmm_region *prev_node = NULL;
    while (cur_node != NULL)
    {
        // ASSUME REGIONS IN ORDER CAUSE WE COOL :sunglasses:
        if (prev_node == NULL)
        {
            prev_node = cur_node;
            cur_node = cur_node->next;
            continue;
        }
        if ((cur_node->base - (prev_node->base + prev_node->length)) >= align_up(size, 4096) + 0x1000)
        {
            struct vmm_region *new_guy = kmalloc(sizeof(struct vmm_region));
            new_guy->base = (prev_node->base + prev_node->length);
            new_guy->length = align_up(size, 4096);
            prev_node->next = new_guy;
            new_guy->next = cur_node;
            cur_node = cur_node->next;
            int num_of_pages = align_up(size, 4096) / 4096; // num of pages to alloc :sunglasses:
            for (int i = 0; i < num_of_pages; i++)
            {
                void *page = pmm_alloc_singlep();
                // memzero that shit!
                memset(page + hhdm_request.response->offset, 0, 4096);
                map((uint64_t)pml4 + hhdm_request.response->offset, new_guy->base + (i * 0x1000), page, flags);
            }
            
            return new_guy->base;

        }
        else
        {
            // not enough space for our new region sadly, continue
            prev_node = cur_node;
            cur_node = cur_node->next;
            continue;
        }
    }
    // STILL HERE?
    // PANIC CAUSE NO REGIONS CAN FIT VMM REGION WOWIE!
    serial_print("NO FREE REGIONS\n");
    asm ("cli");
    for (;;)
    {
        asm ("hlt");
    }
}
uint64_t get_phys_from_entry(uint64_t pte)
{
    return pte & 0x0007FFFFFFFFF000; // get bits 12-51 from the page table entry, to get the physical address
}
void vmm_region_dealloc(uint64_t base)
{
    
    struct vmm_region *cur_node = vmm_head; // find prev node and node after it
    struct vmm_region *prev_prev_node = NULL;
    while(cur_node != NULL)
    {
        if (cur_node->base == base)
        {
            if (cur_node->next)
            {
                prev_prev_node->next = cur_node->next;
            }
            int num_of_pages = cur_node->length / 4096; // assumed to be page aligned
            for (int i = 0; i < num_of_pages; i++)
            {
                uint64_t phys = unmap((uint64_t)pml4 + hhdm_request.response->offset, cur_node->base + (i * 0x1000));
                pmm_free_singlep((uint64_t)get_phys_from_entry(phys) + hhdm_request.response->offset);
            }
            free(cur_node);
            return;
        }
        else
        {
            prev_prev_node = cur_node;
            cur_node = cur_node->next;
            continue;
        }
    }
    // REGION DOESNT EXIST WOWIE!
    write_color(ctx, "Region Provided doesn't exist!\n", 4);
    asm ("cli");
    for (;;)
    {
        asm ("hlt");
    }
}
void vmm_region_walk()
{
    char buffer[64] = "";
    struct vmm_region *cur_node = vmm_head;
    while (cur_node != NULL)
    {
        serial_print_color("Region Information!: Base: ", 0);
        serial_print(itoah(buffer, cur_node->base));
        serial_print(", Length: ");
        serial_print(itoah(buffer, cur_node->length));
        serial_print("\n");
        cur_node = cur_node->next;
    }
}
void vmm_region_setup(uint64_t hhdm_pages)
{
    size_t kernel_size_in_bytes = (size_t)THE_REAL;
    kernel_size_in_bytes = align_up(kernel_size_in_bytes, 4096);
    struct vmm_region *node = kmalloc(sizeof(struct vmm_region));
    node->base = hhdm_request.response->offset;
    node->length = hhdm_pages;
    node->flags = NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW;
    vmm_head = node;
    
    struct vmm_region *kernel_region = kmalloc(sizeof(struct vmm_region));
    kernel_region->base = addr_request.response->virtual_base;
    kernel_region->length = THE_REAL;

    node->next = kernel_region;

    // Regions:
    // -------                          -------
    // - HHDM -                        - KERNEL -
    // -------                          -------
    //       - - - - - - - - - - - - - - ^
    //       next
    serial_print_color("VMM Regions Init!\n", 1);
    vmm_region_walk();
}
void vmm_init()
{
    char shit[64];
    size_t kernel_size_in_bytes = (size_t)THE_REAL;
    kernel_size_in_bytes = align_up(kernel_size_in_bytes, 4096);
    pml4 = pmm_alloc_singlep();
    memset(((uint64_t)pml4 + hhdm_request.response->offset), 0, 4096);
    uint64_t *limine_pagemap = read_cr3();
    limine_pagemap = (uint64_t)limine_pagemap & ~0xFFF;
    // for (int i = 256; i < 512; i++)
    // {
    //     ((uint64_t*)((uint64_t)pml4 + hhdm_request.response->offset))[i] =  ((uint64_t*)((uint64_t)limine_pagemap + hhdm_request.response->offset))[i];
    // }
    for (uint64_t i = 0; i < kernel_size_in_bytes; i += 0x1000)
    {
        map((uint64_t)pml4 + hhdm_request.response->offset, (addr_request.response->virtual_base + i), (addr_request.response->physical_base + i), NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
    }
    uint64_t hhdm_pages = 0;
    for (int i = 0; i < memmap_request.response->entry_count; i++)
    {
        switch(memmap_request.response->entries[i]->type)
        {
            case LIMINE_MEMMAP_USABLE:
                
                for (uint64_t j = 0; j < memmap_request.response->entries[i]->length; j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                }
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                for (uint64_t j = 0; j < memmap_request.response->entries[i]->length; j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                }
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                for (uint64_t j = 0; j < align_up(memmap_request.response->entries[i]->length, 4096); j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                }
                break;
            case LIMINE_MEMMAP_RESERVED:
                for (uint64_t j = 0; j < align_up(memmap_request.response->entries[i]->length, 4096); j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                }
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                for (uint64_t j = 0; j < align_up(memmap_request.response->entries[i]->length, 4096); j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                }
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                for (uint64_t j = 0; j < align_up(memmap_request.response->entries[i]->length, 4096); j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                }
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                for (uint64_t j = 0; j < align_up(memmap_request.response->entries[i]->length, 4096); j += 0x1000)
                {
                    map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + (memmap_request.response->entries[i]->base + j)), memmap_request.response->entries[i]->base + j, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    if (j < 0x100000000)
                    {
                        hhdm_pages += 0x1000;
                    }
                    
                }
                break;
            default:
                break;
        }
        
        
    }
    for (uint64_t i = 0; i < 0x100000000; i += 0x1000)
    {
        map((uint64_t)pml4 + hhdm_request.response->offset, (hhdm_request.response->offset + i), i, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
        hhdm_pages += 0x1000;
    }
    update_cr3(pml4);
    write_color(ctx, "VMM Initialized!\n", 1);
    vmm_region_setup(hhdm_pages);
}