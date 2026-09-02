#include "security.h"
#include "../kernel/terminal.h"

const char *security_role_name(security_role_t role) {
    if (role == ROLE_ADMIN) {
        return "ADMIN";
    }
    return "USER";
}

int security_is_admin(void) {
    return security_is_logged_in() && security_current_role() == ROLE_ADMIN;
}

int security_check_permission(security_role_t required_role) {
    if (!security_is_logged_in()) {
        return 0;
    }
    if (required_role == ROLE_USER) {
        return 1;
    }
    return security_current_role() == ROLE_ADMIN;
}

void security_print_whoami(void) {
    terminal_write("\nUsername: ");
    if (security_is_logged_in()) {
        terminal_write(security_current_username());
    } else {
        terminal_write("NONE");
    }
    terminal_write("\nRole: ");
    terminal_write(security_role_name(security_current_role()));
    terminal_write("\nSession: ");
    terminal_write(security_is_logged_in() ? "ACTIVE" : "INACTIVE");
    terminal_write("\n\n");
}

void security_print_permissions(void) {
    terminal_write("\nRole Permissions\n");
    terminal_write("------------------------------\n");
    terminal_write("USER: notes, tasks, timetable, xp, own files, normal commands\n");
    terminal_write("ADMIN: all USER actions + users, useradd, userdel, passwd management\n\n");
}

void security_print_audit(void) {
    terminal_write("================================\n");
    terminal_write("       STUDYOS SECURITY\n");
    terminal_write("================================\n\n");
    terminal_write("Authentication: ENABLED\n");
    terminal_write("User Sessions: ENABLED\n");
    terminal_write("Roles: ENABLED\n");
    terminal_write("Permissions: ENABLED\n");
    terminal_write("Password Hashing: ENABLED\n");
    terminal_write("Filesystem Protection: ENABLED\n\n");
    terminal_write("Current User: ");
    if (security_is_logged_in()) {
        terminal_write(security_current_username());
    } else {
        terminal_write("NONE");
    }
    terminal_write("\nCurrent Role: ");
    terminal_write(security_role_name(security_current_role()));
    terminal_write("\n================================\n\n");
}
