#include "user.h"

static security_user_t users[SECURITY_MAX_USERS];
static unsigned int next_user_id = 1;

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

void user_store_init(void) {
    unsigned int i = 0;
    for (i = 0; i < SECURITY_MAX_USERS; i++) {
        users[i].user_id = 0;
        users[i].active = 0;
        users[i].username[0] = '\0';
        users[i].password_hash = 0;
        users[i].role = ROLE_USER;
    }
    next_user_id = 1;
}

security_user_t *user_find(const char *username) {
    unsigned int i = 0;
    if (username == 0 || username[0] == '\0') {
        return 0;
    }
    for (i = 0; i < SECURITY_MAX_USERS; i++) {
        if (users[i].active && str_equal(users[i].username, username)) {
            return &users[i];
        }
    }
    return 0;
}

int user_exists(const char *username) {
    return user_find(username) != 0;
}

int user_add(const char *username, unsigned int password_hash, security_role_t role) {
    unsigned int i = 0;
    if (user_find(username) != 0) {
        return 0;
    }

    for (i = 0; i < SECURITY_MAX_USERS; i++) {
        if (!users[i].active) {
            users[i].active = 1;
            users[i].user_id = next_user_id++;
            str_copy(users[i].username, username, SECURITY_USERNAME_MAX);
            users[i].password_hash = password_hash;
            users[i].role = role;
            return 1;
        }
    }
    return 0;
}

int user_remove(const char *username) {
    security_user_t *user = user_find(username);
    if (user == 0) {
        return 0;
    }
    user->active = 0;
    user->user_id = 0;
    user->username[0] = '\0';
    user->password_hash = 0;
    user->role = ROLE_USER;
    return 1;
}

int user_count_active(void) {
    unsigned int i = 0;
    int count = 0;
    for (i = 0; i < SECURITY_MAX_USERS; i++) {
        if (users[i].active) {
            count++;
        }
    }
    return count;
}

int user_count_admins(void) {
    unsigned int i = 0;
    int count = 0;
    for (i = 0; i < SECURITY_MAX_USERS; i++) {
        if (users[i].active && users[i].role == ROLE_ADMIN) {
            count++;
        }
    }
    return count;
}

security_user_t *user_get_at(int index) {
    int seen = 0;
    unsigned int i = 0;
    if (index < 0) {
        return 0;
    }
    for (i = 0; i < SECURITY_MAX_USERS; i++) {
        if (!users[i].active) {
            continue;
        }
        if (seen == index) {
            return &users[i];
        }
        seen++;
    }
    return 0;
}

int user_set_password_hash(const char *username, unsigned int password_hash) {
    security_user_t *user = user_find(username);
    if (user == 0) {
        return 0;
    }
    user->password_hash = password_hash;
    return 1;
}
