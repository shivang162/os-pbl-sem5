#include "terminal.h"

static volatile unsigned short *const VGA_BUFFER = (unsigned short *)0xB8000;
static const unsigned int TERMINAL_WIDTH = 80;
static const unsigned int TERMINAL_HEIGHT = 25;
static unsigned int terminal_row = 0;
static unsigned int terminal_col = 0;
static const unsigned char terminal_color = 0x0F;

static unsigned short vga_entry(char c, unsigned char color) {
    return (unsigned short)c | ((unsigned short)color << 8);
}

static void terminal_scroll(void) {
    unsigned int row;
    unsigned int col;

    if (terminal_row < TERMINAL_HEIGHT) {
        return;
    }

    for (row = 1; row < TERMINAL_HEIGHT; row++) {
        for (col = 0; col < TERMINAL_WIDTH; col++) {
            VGA_BUFFER[(row - 1) * TERMINAL_WIDTH + col] = VGA_BUFFER[row * TERMINAL_WIDTH + col];
        }
    }

    for (col = 0; col < TERMINAL_WIDTH; col++) {
        VGA_BUFFER[(TERMINAL_HEIGHT - 1) * TERMINAL_WIDTH + col] = vga_entry(' ', terminal_color);
    }

    terminal_row = TERMINAL_HEIGHT - 1;
}

void terminal_put_char(char c) {
    if (c == '\n') {
        terminal_row++;
        terminal_col = 0;
        terminal_scroll();
        return;
    }

    VGA_BUFFER[terminal_row * TERMINAL_WIDTH + terminal_col] = vga_entry(c, terminal_color);
    terminal_col++;

    if (terminal_col >= TERMINAL_WIDTH) {
        terminal_col = 0;
        terminal_row++;
        terminal_scroll();
    }
}

void terminal_backspace(void) {
    if (terminal_row == 0 && terminal_col == 0) {
        return;
    }

    if (terminal_col == 0) {
        terminal_row--;
        terminal_col = TERMINAL_WIDTH - 1;
    } else {
        terminal_col--;
    }

    VGA_BUFFER[terminal_row * TERMINAL_WIDTH + terminal_col] = vga_entry(' ', terminal_color);
}

void terminal_clear(void) {
    unsigned int row;
    unsigned int col;

    for (row = 0; row < TERMINAL_HEIGHT; row++) {
        for (col = 0; col < TERMINAL_WIDTH; col++) {
            VGA_BUFFER[row * TERMINAL_WIDTH + col] = vga_entry(' ', terminal_color);
        }
    }

    terminal_row = 0;
    terminal_col = 0;
}

void terminal_write(const char *text) {
    unsigned int i = 0;

    while (text[i] != '\0') {
        terminal_put_char(text[i]);
        i++;
    }
}
