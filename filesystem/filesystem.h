#ifndef STUDYOS_FILESYSTEM_H
#define STUDYOS_FILESYSTEM_H

#define FS_NAME_MAX 64
#define FS_CONTENT_MAX 256
#define FS_LIST_BUFFER_MAX 1024
#define FS_OWNER_MAX 24

#define FS_OK 0
#define FS_ERR_INVALID -1
#define FS_ERR_EXISTS -2
#define FS_ERR_NOT_FOUND -3
#define FS_ERR_FULL -4
#define FS_ERR_NOT_FILE -5
#define FS_ERR_TOO_LARGE -6
#define FS_ERR_PERMISSION -7
#define FS_ERR_NOT_LOGGED_IN -8

void filesystem_init(void);
int filesystem_ls(char *buffer, unsigned int size);
int filesystem_mkdir(const char *name);
int filesystem_touch(const char *name);
int filesystem_cat(const char *name, char *buffer, unsigned int size);
int filesystem_write(const char *name, const char *content);
int filesystem_delete(const char *name);

#endif
