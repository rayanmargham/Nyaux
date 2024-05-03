#include "gdt.h"
#include "main.h"

#define NYA_OS_GDT_PRESENT (1 << 7)
#define NYA_OS_GDT_RING0 (0 << 5)
#define NYA_OS_GDT_RING3 (3 << 5)
#define NYA_OS_GDT_NOSYSTEM (1 << 4)
#define NYA_OS_GDT_EXECUTABLE (1 << 3)
#define NYA_OS_GDT_DC (1 << 2)
#define NYA_OS_GDT_RW (1 << 1)
#define NYA_OS_GDT_ACCESS (1)

#define NYA_OS_GDT_FLAG_LONG_MODE (1 << 5)
#define NYA_OS_GDT_FLAG_PROTECTED_MODE (1 << 6)
extern void gdt_flush(void*);
void create_descriptor(struct GDTDescriptor *des, uint32_t base, uint32_t limit, uint8_t access_byte, uint8_t flags)
{
    des->base_low = (base & 0xFFFF);
    des->base_mid = (base >> 16) & 0xFF;
    des->base_hi = (base >> 24) & 0xFF;

    des->limit_low = (limit & 0xFFFF);
    des->flag_and_hi_limit = ((limit >> 16) & 0x0F);
    des->flag_and_hi_limit |= (flags & 0xF0);
    des->access_byte = access_byte;

}

struct GDTDescriptor gdt[8];

struct GDTR gdtptr;
void init_gdt()
{
    // null
    create_descriptor(&gdt[0], 0x00, 0x00, 0x00, 0x00);
    // 16 bit code kernel
    create_descriptor(&gdt[1], 0x00, 0xFFFF, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING0 | NYA_OS_GDT_EXECUTABLE | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW, 0);

    // 16 bit data kernel
    create_descriptor(&gdt[2], 0x00, 0xFFFF, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING0 | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW | NYA_OS_GDT_ACCESS, 0);

    // 32 bit code kernel
    create_descriptor(&gdt[3], 0x00, 0xFFFFFFFF, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING0 | NYA_OS_GDT_EXECUTABLE | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW, NYA_OS_GDT_FLAG_PROTECTED_MODE);

    // 32 bit data kernel
    create_descriptor(&gdt[4], 0x00, 0xFFFFFFFF, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING0 | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW | NYA_OS_GDT_ACCESS, NYA_OS_GDT_FLAG_PROTECTED_MODE);

    // 64 bit code kernel
    create_descriptor(&gdt[5], 0x00, 0, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING0 | NYA_OS_GDT_EXECUTABLE | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW, NYA_OS_GDT_FLAG_LONG_MODE);

    // 64 bit data kernel
    create_descriptor(&gdt[6], 0x00, 0, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING0 | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW | NYA_OS_GDT_ACCESS, NYA_OS_GDT_FLAG_LONG_MODE);

    // 64 bit code user
    create_descriptor(&gdt[7], 0x00, 0, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING3 | NYA_OS_GDT_EXECUTABLE | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW, NYA_OS_GDT_FLAG_LONG_MODE);

    // 64 bit data user
    create_descriptor(&gdt[8], 0x00, 0, NYA_OS_GDT_PRESENT | NYA_OS_GDT_RING3 | NYA_OS_GDT_NOSYSTEM | NYA_OS_GDT_RW | NYA_OS_GDT_ACCESS, NYA_OS_GDT_FLAG_LONG_MODE);
    
    gdtptr.offset = &gdt;
    gdtptr.size = sizeof(gdt);

    gdt_flush(&gdtptr);
    
}