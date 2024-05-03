#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
int init_serial();
void outb(uint16_t port, uint8_t data);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);
uint32_t ind(uint16_t port);
void outd(uint16_t port, uint32_t data);
void serial_print(char *str);
void serial_print_color(char *str, int type);