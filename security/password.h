#ifndef STUDYOS_PASSWORD_H
#define STUDYOS_PASSWORD_H

unsigned int password_hash(const char *password);
int password_verify(const char *password, unsigned int expected_hash);
int password_is_valid_length(const char *password);

#endif
