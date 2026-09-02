#include "shell.h"
#include "../drivers/keyboard.h"
#include "../filesystem/filesystem.h"
#include "../kernel/terminal.h"
#include "../security/security.h"

static int is_space(char c) {
    return c == ' ' || c == '\t';
}

static int is_printable(char c) {
    return c >= 32 && c <= 126;
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

static void read_input(char *buffer, unsigned int size, int mask) {
    unsigned int length = 0;
    if (size == 0) {
        return;
    }
    buffer[0] = '\0';

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
            buffer[length++] = key;
            buffer[length] = '\0';
            terminal_put_char(mask ? '*' : key);
        }
    }
}

static int require_login(void) {
    if (security_is_logged_in()) {
        return 1;
    }
    terminal_write("\nAccess denied.\n\nPlease login first.\n\n");
    return 0;
}

static int require_admin(void) {
    if (!require_login()) {
        return 0;
    }
    if (security_check_permission(ROLE_ADMIN)) {
        return 1;
    }
    terminal_write("\nPermission denied.\n\nAdministrator privileges required.\n\n");
    return 0;
}

static void write_command_list(void) {
    terminal_write("Available commands:\n\n");
    terminal_write("help         Show available commands\n");
    terminal_write("clear        Clear the screen\n");
    terminal_write("echo         Print text\n");
    terminal_write("about        About StudyOS\n");
    terminal_write("system       Show system information\n");
    terminal_write("version      Show StudyOS version\n");
    terminal_write("login        Login to StudyOS\n");
    terminal_write("logout       Logout current session\n");
    terminal_write("whoami       Show current session user\n");
    terminal_write("security     Show security status\n");
    terminal_write("permissions  Show role permissions\n");
    terminal_write("users        List system users (ADMIN)\n");
    terminal_write("useradd      Add user (ADMIN)\n");
    terminal_write("userdel      Delete user (ADMIN)\n");
    terminal_write("passwd       Change password\n");
    terminal_write("ls           List files and directories\n");
    terminal_write("mkdir        Create a directory\n");
    terminal_write("touch        Create an empty file\n");
    terminal_write("cat          Show file contents\n");
    terminal_write("write        Write text into a file\n");
    terminal_write("delete       Delete a file or directory\n");
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
    } else if (status == FS_ERR_PERMISSION) {
        terminal_write("Permission denied.\n");
    } else if (status == FS_ERR_NOT_LOGGED_IN) {
        terminal_write("Access denied.\nPlease login first.\n");
    } else {
        terminal_write("Filesystem operation failed.\n");
    }
}

static void command_login(void) {
    if (security_is_logged_in()) {
        terminal_write("\nAlready logged in as ");
        terminal_write(security_current_username());
        terminal_write(".\n\n");
        return;
    }
    terminal_write("\n");
    while (!security_is_logged_in()) {
        security_prompt_login();
    }
}

static void command_logout(void) {
    if (!require_login()) {
        return;
    }
    terminal_write("\nLogging out...\n\nSession closed.\n\nPlease login again.\n\n");
    security_logout();
    while (!security_is_logged_in()) {
        security_prompt_login();
    }
}

static void command_users(void) {
    int total;
    int i = 0;

    if (!require_admin()) {
        return;
    }

    terminal_write("\nChecking permissions...\n\nAccess granted.\n\n");
    terminal_write("System Users\n");
    terminal_write("--------------------------\n");
    total = security_user_count();
    for (i = 0; i < total; i++) {
        char username[SECURITY_USERNAME_MAX];
        security_role_t role;
        if (security_get_user_at(i, username, sizeof(username), &role)) {
            terminal_write(username);
            terminal_write("       ");
            terminal_write(security_role_name(role));
            terminal_write("\n");
        }
    }
    terminal_write("\n");
}

static void command_useradd(void) {
    char username[SECURITY_USERNAME_MAX];
    char password[SECURITY_PASSWORD_MAX + 1];
    char role_input[4];
    security_role_t role = ROLE_USER;

    if (!require_admin()) {
        return;
    }

    terminal_write("\nNew username: ");
    read_input(username, sizeof(username), 0);
    terminal_write("Password: ");
    read_input(password, sizeof(password), 1);
    terminal_write("Role:\n1. USER\n2. ADMIN\n\nSelect: ");
    read_input(role_input, sizeof(role_input), 0);

    if (strings_equal(role_input, "2")) {
        role = ROLE_ADMIN;
    } else if (!strings_equal(role_input, "1")) {
        terminal_write("\nInvalid role selection.\n\n");
        return;
    }

    if (!security_add_user(username, password, role)) {
        terminal_write("\nFailed to create user.\nCheck username/password length or existing user.\n\n");
        return;
    }
    terminal_write("\nUser created successfully.\n\n");
}

static void command_userdel(const char *args) {
    char username[SECURITY_USERNAME_MAX];
    char confirm[4];

    if (!require_admin()) {
        return;
    }
    if (!read_token(&args, username, sizeof(username))) {
        terminal_write("\nUsage: userdel <username>\n\n");
        return;
    }

    if (!security_user_exists(username)) {
        terminal_write("\nUser not found.\n\n");
        return;
    }

    terminal_write("\nAre you sure you want to delete user '");
    terminal_write(username);
    terminal_write("'? [Y/N] ");
    read_input(confirm, sizeof(confirm), 0);

    if (!(strings_equal(confirm, "Y") || strings_equal(confirm, "y"))) {
        terminal_write("\nUser deletion cancelled.\n\n");
        return;
    }

    if (!security_delete_user(username)) {
        terminal_write("\nCannot delete user.\nFinal admin and current session are protected.\n\n");
        return;
    }
    terminal_write("\nUser deleted successfully.\n\n");
}

static void command_passwd(const char *args) {
    char target[SECURITY_USERNAME_MAX];
    char current_password[SECURITY_PASSWORD_MAX + 1];
    char new_password[SECURITY_PASSWORD_MAX + 1];
    char confirm_password[SECURITY_PASSWORD_MAX + 1];
    int verify_current = 1;

    if (!require_login()) {
        return;
    }

    if (read_token(&args, target, sizeof(target))) {
        if (!security_is_admin()) {
            terminal_write("\nPermission denied.\n\nAdministrator privileges required.\n\n");
            return;
        }
        verify_current = 0;
    } else {
        const char *self = security_current_username();
        unsigned int i = 0;
        while (self[i] != '\0' && i < sizeof(target) - 1) {
            target[i] = self[i];
            i++;
        }
        target[i] = '\0';
    }

    if (verify_current) {
        terminal_write("\nCurrent password: ");
        read_input(current_password, sizeof(current_password), 1);
    } else {
        current_password[0] = '\0';
    }
    terminal_write("New password: ");
    read_input(new_password, sizeof(new_password), 1);
    terminal_write("Confirm password: ");
    read_input(confirm_password, sizeof(confirm_password), 1);

    if (!strings_equal(new_password, confirm_password)) {
        terminal_write("\nPassword confirmation does not match.\n\n");
        return;
    }

    if (!security_change_password(target, current_password, new_password, verify_current)) {
        terminal_write("\nPassword change failed.\n\n");
        return;
    }
    terminal_write("\nPassword changed successfully.\n\n");
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
    if (strings_equal(command, "login")) {
        command_login();
        return;
    }
    if (strings_equal(command, "logout")) {
        command_logout();
        return;
    }
    if (strings_equal(command, "whoami")) {
        if (require_login()) {
            security_print_whoami();
        }
        return;
    }
    if (strings_equal(command, "security")) {
        if (require_login()) {
            security_print_audit();
        }
        return;
    }
    if (strings_equal(command, "permissions")) {
        if (require_login()) {
            security_print_permissions();
        }
        return;
    }
    if (strings_equal(command, "users")) {
        command_users();
        return;
    }
    if (strings_equal(command, "useradd")) {
        command_useradd();
        return;
    }
    if (strings_equal(command, "userdel")) {
        command_userdel(cursor);
        return;
    }
    if (strings_equal(command, "passwd")) {
        command_passwd(cursor);
        return;
    }

    if (!require_login()) {
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
        terminal_write("Shell: StudyShell\n");
        terminal_write("Security: Phase 8 educational auth system\n\n");
        return;
    }
    if (strings_equal(command, "version")) {
        terminal_write("\nStudyOS Kernel v0.1\n\n");
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
