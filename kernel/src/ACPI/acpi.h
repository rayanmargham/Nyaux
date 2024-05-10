#pragma once
#include <stdint.h>
#include <stddef.h>
void acpi_init();
void writemsr(uint32_t msr, uint64_t val);
void readmsr(uint32_t msr, uint64_t *val);