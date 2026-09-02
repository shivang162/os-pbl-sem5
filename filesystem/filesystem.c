#include "filesystem.h"
#include "../security/security.h"

typedef struct {
    int used;
    int is_directory;
    char owner[FS_OWNER_MAX];
    char name[FS_NAME_MAX];
    char content[FS_CONTENT_MAX];
} fs_entry_t;

#define FS_MAX_ENTRIES 32

static fs_entry_t fs_entries[FS_MAX_ENTRIES];

static unsigned int str_len(const char *text) {
    unsigned int length = 0;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

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

static int str_starts_with(const char *text, const char *prefix) {
    unsigned int i = 0;
    while (prefix[i] != '\0') {
        if (text[i] != prefix[i]) {
            return 0;
        }
        i++;
    }
    return 1;
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

static int owner_can_access(const fs_entry_t *entry) {
    if (security_is_admin()) {
        return 1;
    }
    if (!security_is_logged_in()) {
        return 0;
    }
    return str_equal(entry->owner, security_current_username());
}

static int can_create_in_path(const char *name) {
    unsigned int i = 0;
    char path_user[FS_OWNER_MAX];
    const char *prefix = "/home/";

    if (!security_is_logged_in()) {
        return 0;
    }
    if (security_is_admin()) {
        return 1;
    }
    if (!str_starts_with(name, prefix)) {
        return 1;
    }

    i = 6;
    while (name[i] != '\0' && name[i] != '/' && (i - 6) < FS_OWNER_MAX - 1) {
        path_user[i - 6] = name[i];
        i++;
    }
    path_user[i - 6] = '\0';
    if (path_user[0] == '\0') {
        return 0;
    }
    return str_equal(path_user, security_current_username());
}

static int is_name_valid(const char *name) {
    unsigned int i = 0;
    if (name == 0 || name[0] == '\0') {
        return 0;
    }
    while (name[i] != '\0') {
        if (name[i] == ' ' || name[i] == '\t' || name[i] == '\n') {
            return 0;
        }
        i++;
    }
    return i < FS_NAME_MAX;
}

static int find_entry(const char *name) {
    unsigned int i = 0;
    for (i = 0; i < FS_MAX_ENTRIES; i++) {
        if (fs_entries[i].used && str_equal(fs_entries[i].name, name)) {
            return (int)i;
        }
    }
    return -1;
}

static int find_free_entry(void) {
    unsigned int i = 0;
    for (i = 0; i < FS_MAX_ENTRIES; i++) {
        if (!fs_entries[i].used) {
            return (int)i;
        }
    }
    return -1;
}

static int append_text(char *buffer, unsigned int size, unsigned int *length, const char *text) {
    unsigned int i = 0;
    while (text[i] != '\0') {
        if (*length >= size - 1) {
            return 0;
        }
        buffer[*length] = text[i];
        (*length)++;
        i++;
    }
    buffer[*length] = '\0';
    return 1;
}

void filesystem_init(void) {
    unsigned int i = 0;
    for (i = 0; i < FS_MAX_ENTRIES; i++) {
        fs_entries[i].used = 0;
        fs_entries[i].is_directory = 0;
        fs_entries[i].owner[0] = '\0';
        fs_entries[i].name[0] = '\0';
        fs_entries[i].content[0] = '\0';
    }
}

int filesystem_ls(char *buffer, unsigned int size) {
    unsigned int i = 0;
    unsigned int length = 0;
    int has_any = 0;

    if (buffer == 0 || size == 0) {
        return FS_ERR_INVALID;
    }
    if (!security_is_logged_in()) {
        return FS_ERR_NOT_LOGGED_IN;
    }

    buffer[0] = '\0';
    for (i = 0; i < FS_MAX_ENTRIES; i++) {
        if (!fs_entries[i].used || !owner_can_access(&fs_entries[i])) {
            continue;
        }

        has_any = 1;
        if (fs_entries[i].is_directory) {
            if (!append_text(buffer, size, &length, "[DIR]  ")) {
                return FS_ERR_TOO_LARGE;
            }
        } else {
            if (!append_text(buffer, size, &length, "[FILE] ")) {
                return FS_ERR_TOO_LARGE;
            }
        }

        if (!append_text(buffer, size, &length, fs_entries[i].name)) {
            return FS_ERR_TOO_LARGE;
        }

        if (security_is_admin()) {
            if (!append_text(buffer, size, &length, " (owner: ")) {
                return FS_ERR_TOO_LARGE;
            }
            if (!append_text(buffer, size, &length, fs_entries[i].owner)) {
                return FS_ERR_TOO_LARGE;
            }
            if (!append_text(buffer, size, &length, ")")) {
                return FS_ERR_TOO_LARGE;
            }
        }

        if (fs_entries[i].is_directory) {
            if (!append_text(buffer, size, &length, "/")) {
                return FS_ERR_TOO_LARGE;
            }
        }

        if (!append_text(buffer, size, &length, "\n")) {
            return FS_ERR_TOO_LARGE;
        }
    }

    if (!has_any) {
        str_copy(buffer, "No files or directories found.\n", size);
    }
    return FS_OK;
}

int filesystem_mkdir(const char *name) {
    int free_slot = 0;
    if (!security_is_logged_in()) {
        return FS_ERR_NOT_LOGGED_IN;
    }
    if (!is_name_valid(name)) {
        return FS_ERR_INVALID;
    }
    if (!can_create_in_path(name)) {
        return FS_ERR_PERMISSION;
    }
    if (find_entry(name) >= 0) {
        return FS_ERR_EXISTS;
    }
    free_slot = find_free_entry();
    if (free_slot < 0) {
        return FS_ERR_FULL;
    }

    fs_entries[free_slot].used = 1;
    fs_entries[free_slot].is_directory = 1;
    str_copy(fs_entries[free_slot].owner, security_current_username(), FS_OWNER_MAX);
    str_copy(fs_entries[free_slot].name, name, FS_NAME_MAX);
    fs_entries[free_slot].content[0] = '\0';
    return FS_OK;
}

int filesystem_touch(const char *name) {
    int free_slot = 0;
    if (!security_is_logged_in()) {
        return FS_ERR_NOT_LOGGED_IN;
    }
    if (!is_name_valid(name)) {
        return FS_ERR_INVALID;
    }
    if (!can_create_in_path(name)) {
        return FS_ERR_PERMISSION;
    }
    if (find_entry(name) >= 0) {
        return FS_ERR_EXISTS;
    }
    free_slot = find_free_entry();
    if (free_slot < 0) {
        return FS_ERR_FULL;
    }

    fs_entries[free_slot].used = 1;
    fs_entries[free_slot].is_directory = 0;
    str_copy(fs_entries[free_slot].owner, security_current_username(), FS_OWNER_MAX);
    str_copy(fs_entries[free_slot].name, name, FS_NAME_MAX);
    fs_entries[free_slot].content[0] = '\0';
    return FS_OK;
}

int filesystem_cat(const char *name, char *buffer, unsigned int size) {
    int index = 0;
    if (!security_is_logged_in()) {
        return FS_ERR_NOT_LOGGED_IN;
    }
    if (!is_name_valid(name) || buffer == 0 || size == 0) {
        return FS_ERR_INVALID;
    }

    index = find_entry(name);
    if (index < 0) {
        return FS_ERR_NOT_FOUND;
    }
    if (!owner_can_access(&fs_entries[index])) {
        return FS_ERR_PERMISSION;
    }
    if (fs_entries[index].is_directory) {
        return FS_ERR_NOT_FILE;
    }

    str_copy(buffer, fs_entries[index].content, size);
    return FS_OK;
}

int filesystem_write(const char *name, const char *content) {
    int index = 0;
    if (!security_is_logged_in()) {
        return FS_ERR_NOT_LOGGED_IN;
    }
    if (!is_name_valid(name) || content == 0) {
        return FS_ERR_INVALID;
    }

    index = find_entry(name);
    if (index < 0) {
        return FS_ERR_NOT_FOUND;
    }
    if (!owner_can_access(&fs_entries[index])) {
        return FS_ERR_PERMISSION;
    }
    if (fs_entries[index].is_directory) {
        return FS_ERR_NOT_FILE;
    }
    if (str_len(content) >= FS_CONTENT_MAX) {
        return FS_ERR_TOO_LARGE;
    }

    str_copy(fs_entries[index].content, content, FS_CONTENT_MAX);
    return FS_OK;
}

int filesystem_delete(const char *name) {
    int index = 0;
    if (!security_is_logged_in()) {
        return FS_ERR_NOT_LOGGED_IN;
    }
    if (!is_name_valid(name)) {
        return FS_ERR_INVALID;
    }

    index = find_entry(name);
    if (index < 0) {
        return FS_ERR_NOT_FOUND;
    }
    if (!owner_can_access(&fs_entries[index])) {
        return FS_ERR_PERMISSION;
    }

    fs_entries[index].used = 0;
    fs_entries[index].is_directory = 0;
    fs_entries[index].owner[0] = '\0';
    fs_entries[index].name[0] = '\0';
    fs_entries[index].content[0] = '\0';
    return FS_OK;
}
