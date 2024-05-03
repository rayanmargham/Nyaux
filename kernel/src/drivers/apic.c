#include <main.h>
#include "apic.h"
#include <drivers/serial.h>
#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/acpi.h>
#include <vmm.h>
#include <pmm.h>
struct acpi_madt_ioapic *ioapic;
uint32_t lapic_addr;
uint64_t hpet_addrs;
uint64_t time_per_tick_hpet;
uint64_t lapic_ticks_per_10ms;
volatile void select_ioapic_register(uint8_t ioapic_reg)
{
    // SELECTION REGISTER HAS OFFSET 0
    *(volatile uint32_t*)(ioapic->address + hhdm_request.response->offset) = ioapic_reg;
}
volatile void write_ioapic_selectedregister(uint32_t data)
{
    // READ WRITE REGISTER HAS OFFSET 10h
    *(volatile uint32_t*)(ioapic->address + 0x10 + hhdm_request.response->offset) = data;
}
volatile uint32_t read_ioapic_selectedregister()
{
    // READ WRITE REGISTER HAS OFFSET 10h
    return *(volatile uint32_t*)(ioapic->address + 0x10 + hhdm_request.response->offset);
}

volatile uint32_t read_lapic_register(uint32_t lapic_base, uint16_t reg_offset)
{
    return *(volatile uint32_t*)(lapic_base + reg_offset + hhdm_request.response->offset);
}
volatile void write_lapic_register(uint32_t lapic_base, uint16_t reg_offset, uint32_t data)
{
    *(volatile uint32_t*)(lapic_base + reg_offset + hhdm_request.response->offset) = data;
}
void send_lapic_eoi()
{
    write_lapic_register(lapic_addr, 0xb0, 0);
}
void ksleep(uint64_t ms)
{
    volatile uint64_t poll_start = *(volatile uint64_t*)(hpet_addrs + hhdm_request.response->offset + 0x0f0);
    volatile uint64_t *poll_cur = (volatile uint64_t*)(hpet_addrs + hhdm_request.response->offset + 0x0f0);
    while ((*poll_cur - poll_start) * time_per_tick_hpet < ms * 1000000)
    {
        
    }
}
void apic_init()
{
    // ?
    // Get MADT table i suppose?
    char wow[64];
    uacpi_table *table;
    uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &table);
    struct acpi_madt *madt = table->virt_addr;
    serial_print("local apic addr?: ");
    serial_print(itoah(wow, madt->local_interrupt_controller_address));
    serial_print("\n");
    uint64_t length_of_entries = madt->hdr.length - sizeof(*madt);
    size_t offset = 0;
    while (offset < length_of_entries)
    {
        struct acpi_entry_hdr *ent = (void*)madt->entries + offset;
        serial_print("WE GOT AN ENTRY!: TYPE!: ");
        serial_print(itoa(wow, ent->type));
        serial_print("\n");
        switch (ent->type)
        {
            case ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE:
                serial_print("COMING FROM BUS: ");
                
                struct acpi_madt_interrupt_source_override *ohcool = ent;
                serial_print("BUS: ");
                serial_print(itoa(wow, ohcool->bus));
                serial_print(" IRQ SOURCE: ");
                serial_print(itoa(wow, ohcool->source));
                serial_print(" GSI: ");
                serial_print(itoa(wow, ohcool->gsi));
                serial_print("\n");
                break;
            case ACPI_MADT_ENTRY_TYPE_IOAPIC:
                struct acpi_madt_ioapic *c = ent;
                ioapic = c;
                break;
            case ACPI_MADT_ENTRY_TYPE_LAPIC_NMI:
                struct acpi_madt_lapic_nmi *e = ent;
                serial_print("LOCAL APIC LINT THIS NMI IS CONNECTED TO: ");
                serial_print(itoa(wow, e->lint));
                serial_print(" FOR PROCESSER ID: ");
                serial_print(itoah(wow, e->uid));
                serial_print("\n");
            default:
                break;
        }
        offset += ent->length;
    }
    // ENABLE LAPIC LOL
    // 
    asm("sti");
    asm("mov %0, %%cr8" :: "r"((uint64_t)0));
    lapic_addr = madt->local_interrupt_controller_address;
    serial_print("LAPIC ADDR: ");
    serial_print(itoah(wow, lapic_addr));
    serial_print("\n");
    write_lapic_register(lapic_addr, 0xf0, 0x100 | 33);
    serial_print("enabled lapic?\n");
    // enable lapic timer?
    // divide by 4
    write_lapic_register(lapic_addr, 0x3e0, 1);
    // TIMER, set vector to 34 , ONE SHOT, MASK IT
    write_lapic_register(lapic_addr, 0x320, 34 | (1 << 16));
    // set inital count?
    uacpi_table *h;
    uacpi_status st = uacpi_table_find_by_signature(ACPI_HPET_SIGNATURE, &h);
    if (st != UACPI_STATUS_OK)
    {
        serial_print("HPET DOESNT EXIST, SHIT MY PANTS WE GIVE UP\n");
        write_color(ctx, "SHIT MY PANTS, NO HPET FUCK THIS\n", 4);
        asm("cli"); // disable interrupts
        for (;;)
        {
            asm ("hlt");
        }
    }
    struct acpi_hpet *hpet = h->virt_addr;
    if (hpet->address.address_space_id != ACPI_AS_ID_SYS_MEM)
    {
        serial_print("HPET ISNT MEMORY MAPPED IO, SHIT MY PANTS WE GIVE UP\n");
        write_color(ctx, "SHIT MY PANTS, NOT MEMORY MAPPED IO FUCK THIS\n", 4);
        asm("cli"); // disable interrupts
        for (;;)
        {
            asm ("hlt");
        }
    }
    uint64_t hpet_base = hpet->address.address;
    hpet_addrs = hpet_base;
    serial_print("HPET COUNTER SIZE: ");
    volatile uint32_t config_and_id = *(volatile uint32_t*)(hpet_base + hhdm_request.response->offset);
    if (config_and_id & (1 << 13))
    {
        serial_print("CAPABLE\n");
    }
    else
    {
        serial_print("NOT 64 BIT HPET DETECTED!\n");
        serial_print("HPET ISNT 64 BIT, THIS AINT THE 90s\n");
        write_color(ctx, "SHIT MY PANTS, NOT 64BIT HPET DETECTED FUCK THIS\n", 4);
        asm("cli"); // disable interrupts
        for (;;)
        {
            asm ("hlt");
        }
    }
    volatile uint64_t config_reg = *(volatile uint64_t*)(hpet_base + hhdm_request.response->offset + 0x010);
    if (config_reg & 1)
    {
        serial_print("COUNTER IS RUNNING!\n");
    }
    else
    {
        serial_print("NOT RUNNING ENABLING HPET COUNTER...\n");
        *(volatile uint64_t*)(hpet_base + hhdm_request.response->offset + 0x010) = 1;
    }
    
    volatile uint64_t time_per_tick = ((*(volatile uint64_t*)(hpet_base + hhdm_request.response->offset)) >> 32) & 0xFFFFFFFF;
    time_per_tick = time_per_tick / 1000000; // TIME IN NANOSECONDS
    time_per_tick_hpet = time_per_tick;
    
    write_lapic_register(lapic_addr, 0x380, 0xffffffff);
    ksleep(10);
    volatile uint32_t cur_lapic_count = read_lapic_register(lapic_addr, 0x390);
    uint32_t lapic_ticks_per_10ms = 0xffffffff - cur_lapic_count;
    serial_print("LAPIC TICKS PER 10MS: ");
    serial_print(itoa(wow, lapic_ticks_per_10ms));
    serial_print("\n");
    // DO NOTHING LOL
    serial_print("FINISHED WAITING 10MS\n");
    // PERIODIC, UNMASKED, VECTOR 34, TICK EVERY 10 MS LOL
    write_lapic_register(lapic_addr, 0x380, lapic_ticks_per_10ms);
    write_lapic_register(lapic_addr, 0x320, 32 | (0 << 16) | (1 << 17));
    for (;;)
    {
        // Do nothing lol
    }


}
