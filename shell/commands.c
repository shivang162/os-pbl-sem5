#include "shell.h"
#include "../filesystem/filesystem.h"
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
    terminal_write("ls         List files and directories\n");
    terminal_write("mkdir      Create a directory\n");
    terminal_write("touch      Create an empty file\n");
    terminal_write("cat        Show file contents\n");
    terminal_write("write      Write text into a file\n");
    terminal_write("delete     Delete a file or directory\n");
}

static void skip_spaces(const char **cursor) {
    while (is_space(**cursor)) {
        (*cursor)++;
    }
}

static int read_token(const char **cursor, char *token, unsigned int size) {
    unsigned int i = 0;
    skip_spaces(cursor);

    if (**cursor == '\0' || size == 0) {
        return 0;
    }

    while (**cursor != '\0' && !is_space(**cursor) && i < size - 1) {
        token[i++] = **cursor;
        (*cursor)++;
    }
    token[i] = '\0';

    while (**cursor != '\0' && !is_space(**cursor)) {
        (*cursor)++;
    }

    return i > 0;
}

static void write_fs_error(int status) {
    if (status == FS_ERR_INVALID) {
        terminal_write("Invalid command usage.\n");
    } else if (status == FS_ERR_EXISTS) {
        terminal_write("Name already exists.\n");
    } else if (status == FS_ERR_NOT_FOUND) {
        terminal_write("Entry not found.\n");
    } else if (status == FS_ERR_FULL) {
        terminal_write("Filesystem is full.\n");
    } else if (status == FS_ERR_NOT_FILE) {
        terminal_write("Target is a directory, not a file.\n");
    } else if (status == FS_ERR_TOO_LARGE) {
        terminal_write("Content or output is too large.\n");
    } else {
        terminal_write("Filesystem operation failed.\n");
    }
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

    if (strings_equal(command, "ls")) {
        char list_buffer[FS_LIST_BUFFER_MAX];
        int status = filesystem_ls(list_buffer, sizeof(list_buffer));
        terminal_write("\n");
        if (status == FS_OK) {
            terminal_write(list_buffer);
        } else {
            write_fs_error(status);
        }
        terminal_write("\n");
        return;
    }

    if (strings_equal(command, "mkdir")) {
        char name[FS_NAME_MAX];
        int status;
        if (!read_token(&cursor, name, sizeof(name))) {
            terminal_write("\nUsage: mkdir <name>\n\n");
            return;
        }
        status = filesystem_mkdir(name);
        terminal_write("\n");
        if (status == FS_OK) {
            terminal_write("Directory created.\n");
        } else {
            write_fs_error(status);
        }
        terminal_write("\n");
        return;
    }

    if (strings_equal(command, "touch")) {
        char name[FS_NAME_MAX];
        int status;
        if (!read_token(&cursor, name, sizeof(name))) {
            terminal_write("\nUsage: touch <name>\n\n");
            return;
        }
        status = filesystem_touch(name);
        terminal_write("\n");
        if (status == FS_OK) {
            terminal_write("File created.\n");
        } else {
            write_fs_error(status);
        }
        terminal_write("\n");
        return;
    }

    if (strings_equal(command, "cat")) {
        char name[FS_NAME_MAX];
        char content[FS_CONTENT_MAX];
        int status;
        if (!read_token(&cursor, name, sizeof(name))) {
            terminal_write("\nUsage: cat <file>\n\n");
            return;
        }
        status = filesystem_cat(name, content, sizeof(content));
        terminal_write("\n");
        if (status == FS_OK) {
            terminal_write(content);
            terminal_write("\n");
        } else {
            write_fs_error(status);
        }
        terminal_write("\n");
        return;
    }

    if (strings_equal(command, "write")) {
        char name[FS_NAME_MAX];
        int status;
        if (!read_token(&cursor, name, sizeof(name))) {
            terminal_write("\nUsage: write <file> <text>\n\n");
            return;
        }
        skip_spaces(&cursor);
        status = filesystem_write(name, cursor);
        terminal_write("\n");
        if (status == FS_OK) {
            terminal_write("File written.\n");
        } else {
            write_fs_error(status);
        }
        terminal_write("\n");
        return;
    }

    if (strings_equal(command, "delete")) {
        char name[FS_NAME_MAX];
        int status;
        if (!read_token(&cursor, name, sizeof(name))) {
            terminal_write("\nUsage: delete <name>\n\n");
            return;
        }
        status = filesystem_delete(name);
        terminal_write("\n");
        if (status == FS_OK) {
            terminal_write("Entry deleted.\n");
        } else {
            write_fs_error(status);
        }
        terminal_write("\n");
        return;
    }

    terminal_write("\nUnknown command: ");
    terminal_write(command);
    terminal_write("\nType 'help' for available commands.\n\n");
}
