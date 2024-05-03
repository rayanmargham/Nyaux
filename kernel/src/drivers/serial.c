#include "main.h"
#include "serial.h"

#define PORT 0x3F8
void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %1, %0" : : "dN" (port), "a" (data));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}
void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}
// ind and outd
uint32_t ind(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}
void outd(uint16_t port, uint32_t data) {
    __asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (data));
}
int init_serial()
{
    outb(PORT + 1, 0x00); // DISABLE INTERRUPTS
    outb(PORT + 3, 0x80); // ENABLE DLAB (set band rate divisor)
    outb(PORT + 0, 0xC); // SET DIVISOR TO 12
    outb(PORT + 1, 0x00); 
    outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    write_color(ctx, "Serial Driver Initialized!\n", 1);
    serial_print("\e[40m\033[H"); // clear serial
    serial_print_color("Serial Driver Initialized!\n", 1);
    return 0;
}

void print_charserial(char chara)
{
    outb(PORT, chara);
}

void serial_print_color(char *str, int type)
{
    switch (type)
    {
        case 0: // INFO
            serial_print("\e[40m[\e[0;35mINFO\e[0;37m\e[40m] ");
            serial_print(str);
            break;
        case 1: // OKAY
            serial_print("\e[40m[\e[0;32mOK\e[0;37m\e[40m] ");
            serial_print(str);
            break;
        case 2: // WARNING
            serial_print("\e[40m[\e[0;33mWARNING\e[0;37m\e[40m] ");
            serial_print(str);
            break;
        case 3: // ERROR
            serial_print("\e[40m[\e[0;31mERROR\e[0;37m\e[40m] ");
            serial_print(str);
            break;
        case 4:
            serial_print("\e[41m");
            serial_print("KPANIC: ");
            serial_print(str);
            serial_print("\e[40m");
            break;
    }
}
void serial_print(char *str)
{
    while (*str)
    {
        print_charserial(*str++);
    }
}