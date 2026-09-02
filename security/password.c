#include "password.h"
#include "security.h"

static unsigned int str_len(const char *text) {
    unsigned int length = 0;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

unsigned int password_hash(const char *password) {
    unsigned int hash = 2166136261u;
    unsigned int i = 0;

    while (password[i] != '\0') {
        hash ^= (unsigned int)(unsigned char)password[i];
        hash *= 16777619u;
        hash ^= (hash >> 13);
        i++;
    }

    return hash ^ 0x9E3779B9u;
}

int password_verify(const char *password, unsigned int expected_hash) {
    if (password == 0) {
        return 0;
    }
    return password_hash(password) == expected_hash;
}

int password_is_valid_length(const char *password) {
    unsigned int length;
    if (password == 0) {
        return 0;
    }
    length = str_len(password);
    return length >= SECURITY_PASSWORD_MIN && length <= SECURITY_PASSWORD_MAX;
}
