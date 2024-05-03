#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
void apic_init();
void send_lapic_eoi();
void ksleep(uint64_t ms);