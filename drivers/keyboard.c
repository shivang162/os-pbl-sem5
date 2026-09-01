#include "keyboard.h"
#include <stdint.h>

static const char SCANCODE_MAP[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '\b','\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n',0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0,  '\\','z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*',
    0,   ' ', 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
};

static uint8_t port_read(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void keyboard_init(void) {
    /* Polling-based keyboard setup for StudyOS shell input. */
}

int keyboard_has_input(void) {
    return (port_read(0x64) & 0x01) != 0;
}

char keyboard_read_char(void) {
    while (1) {
        if (!keyboard_has_input()) {
            continue;
        }

        uint8_t scancode = port_read(0x60);
        if (scancode & 0x80) {
            continue;
        }

        if (scancode < 128) {
            char mapped = SCANCODE_MAP[scancode];
            if (mapped != 0) {
                return mapped;
            }
        }
    }
}
