#ifndef STUDYOS_USER_H
#define STUDYOS_USER_H

#include "security.h"

typedef struct {
    unsigned int user_id;
    int active;
    char username[SECURITY_USERNAME_MAX];
    unsigned int password_hash;
    security_role_t role;
} security_user_t;

void user_store_init(void);
int user_add(const char *username, unsigned int password_hash, security_role_t role);
int user_remove(const char *username);
security_user_t *user_find(const char *username);
int user_exists(const char *username);
int user_count_active(void);
int user_count_admins(void);
security_user_t *user_get_at(int index);
int user_set_password_hash(const char *username, unsigned int password_hash);

#endif
