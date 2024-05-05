#include "pmm.h"
#include "drivers/serial.h"
#include "main.h"
#include "vmm.h"

volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 1
};
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 1
};
struct pmm_node *head;
void pmm_init()
{
    struct pmm_node *prev_node = NULL;
    serial_print_color("PMM Init Starting!\n", 0);
    for (int i = 0; i < memmap_request.response->entry_count; i++)
    {
        switch(memmap_request.response->entries[i]->type)
        {
            case LIMINE_MEMMAP_USABLE:
                if (!head)
                {
                    // Since the HHDM maps the entireity of the physical memory in an idenity map
                    // we can simply add the offset to get the virtual address of a physical address
                    head = memmap_request.response->entries[i]->base + hhdm_request.response->offset;

                }
                for (uint64_t j = 0; j < memmap_request.response->entries[i]->length; j += 0x1000)
                {
                    struct pmm_node *cur_node;
                    cur_node = memmap_request.response->entries[i]->base + j + hhdm_request.response->offset;
                    if (prev_node != NULL)
                    {
                        prev_node->next = cur_node;
                    }
                    prev_node = cur_node;
                }
                prev_node->next = NULL;
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                break;
            case LIMINE_MEMMAP_RESERVED:
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                break;
            default:
                break;
        }
        
        
    }
    char buffer[64] = "";
    uint64_t count = 0;
    struct pmm_node *current = head;
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    serial_print_color("Got Free Pages of Total: ", 0);
    serial_print(itoa(buffer, count));
    serial_print("\n");
    serial_print_color("Setting Up Slabs...\n", 0);
    init_slabs();
    serial_print_color("Slabs Initialized!\n", 1);
}

void *pmm_alloc_singlep()
{
    char shit[64];
    if (!head == NULL)
    {
        struct pmm_node *alloc = head;
        head = head->next;
        return (uint64_t)alloc - hhdm_request.response->offset; // RETURN PHYS ADDR
    }
    else
    {
        // panic lol
        write_color(ctx, "Out of Memory!\n", 4);
        asm ("cli");
        for (;;)
        {
            asm ("hlt");
        }
    }
}
void pmm_free_singlep(void *page)
{
    struct pmm_node *node = page; // ASSUME PAGE IS VIRTUAL
    node->next = head;
    head = page;
}

struct cache kmalloc_cache[8];

void init_cache(struct cache *cache, size_t size)
{
    char buffer[64] = "";
    cache->size = size;
    void *page = pmm_alloc_singlep();
    memset((uint64_t)page + hhdm_request.response->offset, 0, 4096);
    struct Header *h = page + hhdm_request.response->offset;
    h->size = size;
    cache->slabs = (uint64_t)h;
    
    size_t obj_amount = (4096 - sizeof(struct Header)) / (size);
    write_color(ctx, "Slab: Creating Cache of Object Count: ", 0);
    write(ctx, itoa(buffer, obj_amount));
    write(ctx, " and Size: ");
    write(ctx, itoa(buffer, size));
    write(ctx, "\n");
    struct pmm_node *start = (uint64_t)h + sizeof(struct Header);
    h->freelist = start;
    struct pmm_node *prev = start;
    for (int i = 1; i < obj_amount; i++)
    {
        struct pmm_node *new = (uint64_t)start + (i * size);
        prev->next = new;
        prev = new;
    }
    write_color(ctx, "Slab: Cache Created!\n", 0);
}

struct Header *make_slab(size_t size)
{
    char shit[64] = "";
    void *page = pmm_alloc_singlep();
    memset((uint64_t)page + hhdm_request.response->offset, 0, 4096);
    struct Header *h = page + hhdm_request.response->offset;
    h->size = size;
    size_t obj_amount = (4096 - sizeof(struct Header)) / (size);
    struct pmm_node *start = (uint64_t)h + sizeof(struct Header);
    h->freelist = start;
    struct pmm_node *prev = start;
    for (int i = 1; i < obj_amount; i++)
    {
        struct pmm_node *new = (uint64_t)start + (i * size);
        prev->next = new;
        prev = new;
    }
    return h;
}
size_t next_pow2(size_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
static inline struct cache *search_for_bigenoughcache(size_t size)
{
    for (int i = 0; i < 8; i++)
    {
        if (kmalloc_cache[i].size >= size)
        {
            return &kmalloc_cache[i];
        }
    }
    return NULL;
}
void *slab_alloc(struct cache *cache)
{
    struct Header *h = cache->slabs;
    while (h != NULL)
    {
        if (h->freelist != NULL)
        {
            char shit[64];
            struct pmm_node *him = h->freelist;
            h->freelist = him->next;
            return him;
        }
        else
        {
            if (h->next == NULL)
            {
                break;
            }
            h = h->next; // go to next header
        }
    }
    // no slabs at all on this cache
    // allocate new slab
    struct Header *new_slab = make_slab(cache->size);
    h->next = new_slab;
    struct pmm_node *we = new_slab->freelist;
    new_slab->freelist = new_slab->freelist->next;
    char shit[64];
    return we;
}
void free(void *addr)
{
    char da[64] = "";
    if ((uint64_t)addr == 0)
    {
        return;
    }
    struct Header *h = (uint64_t)addr & ~0xFFF;
    struct cache *rightcache = NULL;
    for (int i = 0; i < 8; i++)
    {
        if (kmalloc_cache[i].size == h->size)
        {
            rightcache = &kmalloc_cache[i];
        }
    }
    if (rightcache == NULL)
    {
        return;
    }
    struct pmm_node *new = (uint64_t)addr;
    new->next = h->freelist;
    h->freelist = new;
    // put header into header freelist if not
    struct Header *prev = NULL;
    struct Header *shit = rightcache->slabs;
    while (shit != NULL)
    {
        if (shit == h)
        {
            return; // no need to do anything
        }
        else
        {
            prev = shit;
            shit = shit->next;
        }
    }
    // slab was fully used and now needs to be put back onto the header list
    prev->next = h;
    return;
}
void kfree(void *addr, size_t size)
{
    if (addr != NULL)
    {
        if (size < 1024)
        {
            free(addr);
        }
        else
        {
            vmm_region_dealloc(addr);
        }
    }
}
void *kmalloc(size_t size)
{
    if (size < 1024)
    {
        struct cache *ca = search_for_bigenoughcache(next_pow2(size));
        if (ca == NULL)
        {
            serial_print("COUDNLT FIND FOR THIS SIZE BOZO!\n");
            return;
        }
        if (ca->size < size)
        {
            serial_print("THATS OUR ERROR\n");
        }
        return slab_alloc(ca);
    }
    else
    {
        return vmm_region_alloc(align_up(size, 4096), NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
    }
}
void *krealloc(void *addr, size_t oldsz, size_t newsz)
{
    void *new_shit = kmalloc(newsz);
    memcpy(new_shit, addr, oldsz);
    kfree(addr, oldsz);
    return new_shit;
}
void init_slabs()
{
    init_cache(&kmalloc_cache[0], 8);
    init_cache(&kmalloc_cache[1], 16);
    init_cache(&kmalloc_cache[2], 32);
    init_cache(&kmalloc_cache[3], 64);
    init_cache(&kmalloc_cache[4], 128);
    init_cache(&kmalloc_cache[5], 256);
    init_cache(&kmalloc_cache[6], 512);
    init_cache(&kmalloc_cache[7], 1024);
}