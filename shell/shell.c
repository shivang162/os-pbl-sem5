#include "shell.h"
#include "../drivers/keyboard.h"
#include "../kernel/terminal.h"
#include "../security/security.h"

static int is_printable(char c) {
    return c >= 32 && c <= 126;
}

static void shell_print_prompt(void) {
    terminal_write("studyos> ");
}

static void shell_read_line(char *buffer, unsigned int size) {
    unsigned int length = 0;

    while (1) {
        char key = keyboard_read_char();

        if (key == '\n') {
            terminal_put_char('\n');
            break;
        }

        if (key == '\b') {
            if (length > 0) {
                length--;
                buffer[length] = '\0';
                terminal_backspace();
            }
            continue;
        }

        if (!is_printable(key)) {
            continue;
        }

        if (length < size - 1) {
            buffer[length] = key;
            length++;
            buffer[length] = '\0';
            terminal_put_char(key);
        }
    }
}

void shell_start(void) {
    char command_buffer[128];

    terminal_write("========================================\n");
    terminal_write("              STUDYOS\n");
    terminal_write("       AI-POWERED STUDENT OS\n");
    terminal_write("========================================\n\n");
    while (!security_is_logged_in()) {
        security_prompt_login();
    }

    while (1) {
        while (!security_is_logged_in()) {
            security_prompt_login();
        }
        command_buffer[0] = '\0';
        shell_print_prompt();
        shell_read_line(command_buffer, sizeof(command_buffer));
        shell_execute_command(command_buffer);
    }
}
