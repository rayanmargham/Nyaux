#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
void apic_init();
void send_lapic_eoi();
void ksleep(uint64_t ms);
bool route_irq(uint8_t irq, uint32_t lapic_id, uint8_t vec);
uint32_t get_lapic_id();