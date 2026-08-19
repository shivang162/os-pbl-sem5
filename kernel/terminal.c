#include "terminal.h"

static volatile unsigned short *const VGA_BUFFER = (unsigned short *)0xB8000;
static unsigned int terminal_row = 0;
static unsigned int terminal_col = 0;
static const unsigned char terminal_color = 0x0F;

static unsigned short vga_entry(char c, unsigned char color) {
    return (unsigned short)c | ((unsigned short)color << 8);
}

void terminal_clear(void) {
    unsigned int row;
    unsigned int col;

    for (row = 0; row < 25; row++) {
        for (col = 0; col < 80; col++) {
            VGA_BUFFER[row * 80 + col] = vga_entry(' ', terminal_color);
        }
    }

    terminal_row = 0;
    terminal_col = 0;
}

void terminal_write(const char *text) {
    unsigned int i = 0;

    while (text[i] != '\0') {
        if (text[i] == '\n') {
            terminal_row++;
            terminal_col = 0;
        } else {
            VGA_BUFFER[terminal_row * 80 + terminal_col] = vga_entry(text[i], terminal_color);
            terminal_col++;
            if (terminal_col >= 80) {
                terminal_col = 0;
                terminal_row++;
            }
        }

        if (terminal_row >= 25) {
            terminal_row = 0;
        }
        i++;
    }
}

