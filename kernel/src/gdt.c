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


uint64_t  gdt[11];

struct TSS ts = {};
struct GDTR gdtptr;
void init_gdt()
{
    gdt[0] = 0x0; 
    gdt[1] = 0x00009a000000ffff;
    gdt[2] = 0x000093000000ffff;
    gdt[3] = 0x00cf9a000000ffff;
    gdt[4] = 0x00cf93000000ffff;
    gdt[5] = 0x00af9b000000ffff;
    gdt[6] = 0x00af93000000ffff;
    gdt[7] = 0x00aff3000000ffff;
    gdt[8] = 0x00affb000000ffff;
    gdt[9] = ((uint64_t)0x9 << 40) | ((uint64_t)1 << 47) | ((uint16_t)sizeof(ts) & 0xFFFF) | (((uint64_t)&ts & 0xFFFF) << 16) | ((((uint64_t)&ts >> 16) & 0xFF) << 32) | ((((uint64_t)&ts >> 24) & 0xFF) << 56);
    gdt[10] = ((uint64_t)&ts >> 32);
    gdtptr.offset = &gdt;
    gdtptr.size = sizeof(gdt);
    gdt_flush(&gdtptr);
    
}