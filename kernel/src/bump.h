#pragma once
#include <stdint.h>
#include <stddef.h>

void bump_init();
void *bump_alloc(size_t size);