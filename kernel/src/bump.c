#include "lib/kpanic.h"
#include "main.h"
#include "vmm.h"
#include "pmm.h"
#include "drivers/serial.h"

uint64_t base = 0;
uint64_t max_size = 4096 * 10000;
char meow[64];
void bump_init()
{
    base = 0x100000000 + hhdm_request.response->offset;
    max_size = base + max_size;
}
void *bump_alloc(size_t size)
{
    serial_print("Max Size: ");
    serial_print(itoa(meow, max_size));
    serial_print("\n");
    if (base < max_size && base + align_up(size, 8) < max_size)
    {
        serial_print("bump(): alloc of size ");
        serial_print(itoa(meow, size));
        serial_print("\n");
        
        size_t aligned = align_up(size, 8);
        void *result = base;
        base += aligned;
        return result;
    }
    else
    {
        serial_print("bump(): alloc of size ");
        serial_print(itoa(meow, size));
        serial_print(" Failed\n");
        kpanic("Out of Memory!\n", NULL);
        return NULL;
    }
}