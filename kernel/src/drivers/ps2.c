#include "ps2.h"
#include <main.h>
#include "apic.h"
#include <drivers/serial.h>
#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/acpi.h>
#include <vmm.h>
#include <pmm.h>
#include <idt.h>
#include <lib/kpanic.h>
char us_qwerty_keyboard[] = 
{
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
};
uint8_t key_buf[8];
uint8_t key_buf_count = 0;

volatile struct KeyboardEvent events[32];
volatile int event_count = 0;
void ps2_handler(struct interrupt_frame *frame)
{
    uint8_t our_guy = inb(0x60);
    key_buf[key_buf_count++] = our_guy;
    if (key_buf[0] == 0xE0)
    {
        if (key_buf_count > 1)
        {
            if (key_buf[1] == 0x2A)
            {
                if (key_buf_count > 3 && memcmp(key_buf, "\xe0\x2a\xe0\x37", 4) == 0)
                {
                    // PRINT SCREEN PRESSED
                    key_buf_count -= 4;
                    kassert(event_count < 32);
                    events[event_count++] = (struct KeyboardEvent){
                        .printable = 0,
                        .key = KEY_PRINTSCRN,
                        .released = false,
                    };
                }

            }
            else if (key_buf[1] == 0xB7)
            {
                if (key_buf_count > 3 && memcmp(key_buf, "\xe0\xb7\xe0\xaa", 4) == 0)
                {
                    // PRINT SCREEN RELEASED
                    key_buf_count -= 4;
                    kassert(event_count < 32);
                    events[event_count++] = (struct KeyboardEvent){
                        .printable = 0,
                        .key = KEY_PRINTSCRN,
                        .released = true,
                    };
                }
            }
            else
            {
                // Extended Key.
                key_buf_count -= 2;
                kassert(event_count < 32);
                enum KEYKEY choson = KEY_UNKNOWN;
                switch (key_buf[1] & 0x7F)
                {
                    default:
                        choson = KEY_UNKNOWN;
                        break;
                }
                bool rel = key_buf[1] & 0x80;
                events[event_count++] = (struct KeyboardEvent){
                    .printable = 0,
                    .key = choson,
                    .released = rel,
                };
            }
        }
        
    }
    else if (key_buf[0] == 0xE1)
    {
        if (key_buf_count > 5 && memcmp(key_buf, "\xe1\x1d\x45\xe1\x9d\xc5", 6) == 0)
        {
            // PAUSE PRESSED
            key_buf_count -= 6;
            kassert(event_count + 1 < 32);
            events[event_count++] = (struct KeyboardEvent){
                .printable = 0,
                .key = KEY_PAUSE,
                .released = false,
            };
            events[event_count++] = (struct KeyboardEvent){
                .printable = 0,
                .key = KEY_PAUSE,
                .released = true,
            };
        }
        
    }
    else
    {
        key_buf_count -= 1;
        kassert(event_count < 32);
        bool rel = key_buf[0] & 0x80;
        enum KEYKEY poop = KEY_UNKNOWN;
        char ch = us_qwerty_keyboard[key_buf[0] & 0x7F];
        if (ch == 0)
        {
            switch (key_buf[0] & 0x7F)
            {
                case 0x01:
                    poop = KEY_ESCAPE;
                    break;
                case 0x0E:
                    poop = KEY_BACKSPACE;
                    break;
                case 0x0F:
                    poop = KEY_TAB;
                    break;
                case 0x1C:
                    poop = KEY_ENTER;
                    break;
                case 0x1D:
                    poop = KEY_LCTRL;
                    break;
                case 0x2A:
                    poop = KEY_LSHIFT;
                    break;
                case 0x36:
                    poop = KEY_RSHIFT;
                    break;
                default:
                    poop = KEY_UNKNOWN;
                    break;
            }
        }
        events[event_count++] = (struct KeyboardEvent){
            .printable = ch,
            .key = poop,
            .released = rel,
        };
    }
    send_lapic_eoi();
}
void ps2_init()
{
    kprintf("%p\n", &event_count);
    int choosenone = AllocateVector();
    RegisterHandler(choosenone, ps2_handler);
    route_irq(1, get_lapic_id(), choosenone);
}