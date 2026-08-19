#include "shell.h"
#include "../kernel/terminal.h"

static int is_space(char c) {
    return c == ' ' || c == '\t';
}

static int strings_equal(const char *a, const char *b) {
    unsigned int i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

static void write_command_list(void) {
    terminal_write("Available commands:\n\n");
    terminal_write("help       Show available commands\n");
    terminal_write("clear      Clear the screen\n");
    terminal_write("echo       Print text\n");
    terminal_write("about      About StudyOS\n");
    terminal_write("system     Show system information\n");
    terminal_write("version    Show StudyOS version\n");
}

void shell_execute_command(const char *input) {
    char command[32];
    const char *cursor = input;
    unsigned int index = 0;

    while (is_space(*cursor)) {
        cursor++;
    }

    if (*cursor == '\0') {
        return;
    }

    while (*cursor != '\0' && !is_space(*cursor) && index < sizeof(command) - 1) {
        command[index++] = *cursor++;
    }
    command[index] = '\0';

    while (is_space(*cursor)) {
        cursor++;
    }

    if (strings_equal(command, "help")) {
        terminal_write("\n");
        write_command_list();
        terminal_write("\n");
        return;
    }

    if (strings_equal(command, "clear")) {
        terminal_clear();
        return;
    }

    if (strings_equal(command, "echo")) {
        terminal_write("\n");
        terminal_write(cursor);
        terminal_write("\n\n");
        return;
    }

    if (strings_equal(command, "about")) {
        terminal_write("\n");
        terminal_write("StudyOS\n");
        terminal_write("AI-Powered Student Mini Operating System\n\n");
        terminal_write("Built as a college Computer Science project.\n\n");
        return;
    }

    if (strings_equal(command, "system")) {
        terminal_write("\n");
        terminal_write("System Information\n");
        terminal_write("------------------\n");
        terminal_write("OS: StudyOS\n");
        terminal_write("Architecture: x86\n");
        terminal_write("Kernel: StudyOS Kernel\n");
        terminal_write("Version: 0.1\n");
        terminal_write("Shell: StudyShell\n\n");
        return;
    }

    if (strings_equal(command, "version")) {
        terminal_write("\n");
        terminal_write("StudyOS Kernel v0.1\n\n");
        return;
    }

    terminal_write("\nUnknown command: ");
    terminal_write(command);
    terminal_write("\nType 'help' for available commands.\n\n");
}
