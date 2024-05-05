#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


void ps2_init();
enum KEYKEY
{
    KEY_ESCAPE,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_ENTER,
    KEY_LCTRL,
    KEY_LSHIFT,
    KEY_RSHIFT,
    KEY_LALT,
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_NUMLOCK,
    KEY_SCRLLOCK,
    KEY_KEYPAD7,
    KEY_KEYPAD8,
    KEY_KEYPAD9,
    KEY_KEYPADMINUS,
    KEY_KEYPAD4,
    KEY_KEYPAD5,
    KEY_KEYPAD6,
    KEY_KEYPADPLUS,
    KEY_KEYPAD1,
    KEY_KEYPAD2,
    KEY_KEYPAD3,
    KEY_KEYPAD0,
    KEY_KEYPADDOT,
    KEY_KEYPADENTER,
    KEY_PRINTSCRN,
    KEY_PAUSE,
    KEY_UNKNOWN,
};
struct KeyboardEvent
{
    char printable;
    enum KEYKEY key;
    bool released;
};
extern volatile struct KeyboardEvent events[32];
extern volatile int event_count;