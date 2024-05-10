#include "acpi.h"
#include "main.h"
#include "drivers/serial.h"
#include "vmm.h"
#include "pmm.h"
#include "limine.h"
#include <uacpi/uacpi.h>
void readmsr(uint32_t msr, uint64_t *val)
{
   uint32_t lo = 0;
   uint32_t hi = 0;
   asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
   *val = (uint64_t)lo | ((uint64_t)hi << 32);
}
 
void writemsr(uint32_t msr, uint64_t val)
{
   uint32_t lo = (val) & 0xFFFFFFFF;
   uint32_t hi = (val >> 32) & 0xFFFFFFFF;
   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 1
};
// struct acpi_xsdt_t *xsdt;

// void *scandatable(const char* signaturem, size_t index)
// {
//     int makingsure = 0;
//     int amount_of_entries = (xsdt->header.length - sizeof(acpi_header_t)) / sizeof(uint64_t);
//     for (int i = 0; i < amount_of_entries; i++)
//     {
//         struct acpi_header_t *gamer_table = laihost_map(xsdt->tables[i], 0);
//         if (!memcmp(signaturem, gamer_table->signature, 4))
//         {
//             if (index == makingsure)
//             {
//                 return gamer_table;
//             }
//             else
//             {
//                 makingsure++;
//             }
//         }
//     }
//     return NULL;
// }
// void *GetTable(const char* signature, size_t index)
// {
//     if (!memcmp(signature, "DSDT", 4))
//     {
//         struct acpi_fadt_t *fadt_among_us = scandatable("FACP", 0);

//         return laihost_map(fadt_among_us->x_dsdt, 0);
//     }
//     else
//     {
//         return scandatable(signature, index);
//     }
// }
void acpi_init()
{
   uint64_t rsdp = (uint64_t)rsdp_request.response->address - hhdm_request.response->offset;
   struct uacpi_init_params params = {
      rsdp,
      { UACPI_LOG_INFO, 0 }
   };
   uacpi_status st = uacpi_initialize(&params);
   if (st == UACPI_STATUS_OK)
   {
      st = uacpi_namespace_load();
      st = uacpi_namespace_initialize();
      
   }
   else
   {
      write_color(ctx, "im a failure lol\n", 4);
   }
}