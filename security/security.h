#ifndef STUDYOS_SECURITY_H
#define STUDYOS_SECURITY_H

#define SECURITY_USERNAME_MAX 24
#define SECURITY_PASSWORD_MIN 6
#define SECURITY_PASSWORD_MAX 32
#define SECURITY_MAX_USERS 16
#define SECURITY_MAX_LOGIN_ATTEMPTS 3

typedef enum {
    ROLE_USER = 0,
    ROLE_ADMIN = 1
} security_role_t;

typedef struct {
    int logged_in;
    char username[SECURITY_USERNAME_MAX];
    security_role_t role;
} security_session_t;

void security_init(void);
int security_login(const char *username, const char *password);
void security_logout(void);
int security_prompt_login(void);
int security_is_logged_in(void);
const char *security_current_username(void);
security_role_t security_current_role(void);
const security_session_t *security_get_session(void);

int security_check_permission(security_role_t required_role);
const char *security_role_name(security_role_t role);
int security_is_admin(void);

int security_add_user(const char *username, const char *password, security_role_t role);
int security_delete_user(const char *username);
int security_change_password(const char *username, const char *current_password, const char *new_password, int verify_current_password);
int security_user_count(void);
int security_get_user_at(int index, char *username_out, unsigned int username_out_size, security_role_t *role_out);
int security_admin_count(void);
int security_user_exists(const char *username);

void security_print_whoami(void);
void security_print_permissions(void);
void security_print_audit(void);

#endif
