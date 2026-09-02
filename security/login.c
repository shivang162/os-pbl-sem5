#include "security.h"
#include "password.h"
#include "user.h"
#include "../drivers/keyboard.h"
#include "../kernel/terminal.h"

static security_session_t current_session;

static int str_equal(const char *a, const char *b) {
    unsigned int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static void str_copy(char *dst, const char *src, unsigned int dst_size) {
    unsigned int i = 0;
    if (dst_size == 0) {
        return;
    }
    while (src[i] != '\0' && i < dst_size - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static int is_printable(char c) {
    return c >= 32 && c <= 126;
}

static int username_is_valid(const char *username) {
    unsigned int i = 0;
    if (username == 0 || username[0] == '\0') {
        return 0;
    }

    while (username[i] != '\0') {
        if (is_space(username[i])) {
            return 0;
        }
        i++;
    }
    return i > 0 && i < SECURITY_USERNAME_MAX;
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
            if (mask) {
                terminal_put_char('*');
            } else {
                terminal_put_char(key);
            }
        }
    }
}

void security_init(void) {
    user_store_init();
    current_session.logged_in = 0;
    current_session.username[0] = '\0';
    current_session.role = ROLE_USER;

    security_add_user("admin", "admin123", ROLE_ADMIN);
    security_add_user("student", "student123", ROLE_USER);
}

int security_login(const char *username, const char *password) {
    security_user_t *user;

    if (!username_is_valid(username) || !password_is_valid_length(password)) {
        return 0;
    }

    user = user_find(username);
    if (user == 0 || !password_verify(password, user->password_hash)) {
        return 0;
    }

    current_session.logged_in = 1;
    str_copy(current_session.username, user->username, SECURITY_USERNAME_MAX);
    current_session.role = user->role;
    return 1;
}

void security_logout(void) {
    current_session.logged_in = 0;
    current_session.username[0] = '\0';
    current_session.role = ROLE_USER;
}

int security_prompt_login(void) {
    int attempts = 0;
    char username[SECURITY_USERNAME_MAX];
    char password[SECURITY_PASSWORD_MAX + 1];

    terminal_write("Please login to continue.\n\n");
    while (attempts < SECURITY_MAX_LOGIN_ATTEMPTS) {
        terminal_write("Username: ");
        read_input(username, sizeof(username), 0);
        terminal_write("Password: ");
        read_input(password, sizeof(password), 1);
        terminal_write("\n");

        if (security_login(username, password)) {
            terminal_write("Login successful!\n\nWelcome");
            if (str_equal(current_session.username, "admin")) {
                terminal_write(", admin.\n\n");
            } else {
                terminal_write(" back, ");
                terminal_write(current_session.username);
                terminal_write(".\n\n");
            }
            terminal_write("Role: ");
            terminal_write(security_role_name(current_session.role));
            terminal_write("\n\n");
            return 1;
        }

        attempts++;
        terminal_write("Authentication failed.\n\nInvalid username or password.\n\n");
    }

    terminal_write("Too many failed login attempts.\n\nLogin temporarily locked.\n\n");
    return 0;
}

int security_is_logged_in(void) {
    return current_session.logged_in;
}

const char *security_current_username(void) {
    if (!current_session.logged_in) {
        return "";
    }
    return current_session.username;
}

security_role_t security_current_role(void) {
    return current_session.role;
}

const security_session_t *security_get_session(void) {
    return &current_session;
}

int security_add_user(const char *username, const char *password, security_role_t role) {
    if (!username_is_valid(username) || !password_is_valid_length(password)) {
        return 0;
    }
    return user_add(username, password_hash(password), role);
}

int security_delete_user(const char *username) {
    security_user_t *user;

    if (!username_is_valid(username)) {
        return 0;
    }
    user = user_find(username);
    if (user == 0) {
        return 0;
    }

    if (user->role == ROLE_ADMIN && user_count_admins() <= 1) {
        return 0;
    }
    if (current_session.logged_in && str_equal(current_session.username, username)) {
        return 0;
    }
    return user_remove(username);
}

int security_change_password(const char *username, const char *current_password, const char *new_password, int verify_current_password) {
    security_user_t *user;

    if (!username_is_valid(username) || !password_is_valid_length(new_password)) {
        return 0;
    }
    user = user_find(username);
    if (user == 0) {
        return 0;
    }

    if (verify_current_password && !password_verify(current_password, user->password_hash)) {
        return 0;
    }
    return user_set_password_hash(username, password_hash(new_password));
}

int security_user_count(void) {
    return user_count_active();
}

int security_get_user_at(int index, char *username_out, unsigned int username_out_size, security_role_t *role_out) {
    security_user_t *user = user_get_at(index);
    if (user == 0 || username_out == 0 || username_out_size == 0 || role_out == 0) {
        return 0;
    }
    str_copy(username_out, user->username, username_out_size);
    *role_out = user->role;
    return 1;
}

int security_admin_count(void) {
    return user_count_admins();
}

int security_user_exists(const char *username) {
    return user_exists(username);
}
