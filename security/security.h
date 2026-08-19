#ifndef STUDYOS_SECURITY_H
#define STUDYOS_SECURITY_H

void security_init(void);
int security_login(const char *username, const char *password_hash);
int security_check_permission(const char *username, const char *action);

#endif

