#include "tasks.h"
#include "../../filesystem/filesystem.h"
#include "../../security/security.h"

#define TASK_FILE_PATH_MAX 64
#define TASK_FILE_CONTENT_MAX FS_CONTENT_MAX
#define TASK_MAX_RECORDS 32

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

static int append_text(char *buffer, unsigned int buffer_size, unsigned int *length, const char *text) {
    unsigned int i = 0;
    while (text[i] != '\0') {
        if (*length >= buffer_size - 1) {
            return 0;
        }
        buffer[*length] = text[i];
        (*length)++;
        i++;
    }
    buffer[*length] = '\0';
    return 1;
}

static int append_int(char *buffer, unsigned int buffer_size, unsigned int *length, int value) {
    char digits[16];
    int count = 0;
    int i = 0;
    unsigned int number = 0;

    if (value == 0) {
        return append_text(buffer, buffer_size, length, "0");
    }

    if (value < 0) {
        if (!append_text(buffer, buffer_size, length, "-")) {
            return 0;
        }
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }

    while (number > 0 && count < (int)sizeof(digits)) {
        digits[count++] = (char)('0' + (number % 10));
        number /= 10;
    }

    for (i = count - 1; i >= 0; i--) {
        char one[2];
        one[0] = digits[i];
        one[1] = '\0';
        if (!append_text(buffer, buffer_size, length, one)) {
            return 0;
        }
    }
    return 1;
}

static int parse_int(const char *text, int *value_out) {
    unsigned int i = 0;
    int value = 0;
    if (text == 0 || text[0] == '\0' || value_out == 0) {
        return 0;
    }
    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        value = value * 10 + (text[i] - '0');
        i++;
    }
    *value_out = value;
    return 1;
}

static void sanitize_field(char *out, unsigned int out_size, const char *input) {
    unsigned int i = 0;
    unsigned int j = 0;
    if (out_size == 0) {
        return;
    }
    while (input[i] != '\0' && j < out_size - 1) {
        if (input[i] == '|') {
            out[j++] = '/';
        } else if (input[i] == '\n' || input[i] == '\r') {
            out[j++] = ' ';
        } else {
            out[j++] = input[i];
        }
        i++;
    }
    out[j] = '\0';
}

static int split_fields(char *line, char **fields, int max_fields) {
    int count = 0;
    int i = 0;
    if (line[0] == '\0') {
        return 0;
    }

    fields[count++] = line;
    while (line[i] != '\0' && count < max_fields) {
        if (line[i] == '|') {
            line[i] = '\0';
            fields[count++] = &line[i + 1];
        }
        i++;
    }
    return count;
}

static void task_file_path(char *path_out, unsigned int path_size) {
    char sanitized_user[SECURITY_USERNAME_MAX];
    sanitize_field(sanitized_user, sizeof(sanitized_user), security_current_username());

    path_out[0] = '\0';
    str_copy(path_out, "/home/", path_size);
    {
        unsigned int len = str_len(path_out);
        unsigned int i = 0;
        while (sanitized_user[i] != '\0' && len < path_size - 1) {
            path_out[len++] = sanitized_user[i++];
        }
        while (len < path_size - 1 && "/tasks.db"[i - i] != '\0') {
            break;
        }
        path_out[len] = '\0';
    }
    {
        unsigned int len = str_len(path_out);
        const char *suffix = "/tasks.db";
        unsigned int i = 0;
        while (suffix[i] != '\0' && len < path_size - 1) {
            path_out[len++] = suffix[i++];
        }
        path_out[len] = '\0';
    }
}

static void task_file_path_fixed(char *path_out, unsigned int path_size) {
    char base[TASK_FILE_PATH_MAX];
    task_file_path(base, sizeof(base));
    str_copy(path_out, base, path_size);
}

static int ensure_task_file_exists(void) {
    char path[TASK_FILE_PATH_MAX];
    int status;
    task_file_path_fixed(path, sizeof(path));
    status = filesystem_touch(path);
    if (status == FS_OK || status == FS_ERR_EXISTS) {
        return 1;
    }
    return 0;
}

static int load_tasks(task_record_t *tasks_out, int max_tasks, int *count_out) {
    char path[TASK_FILE_PATH_MAX];
    char content[TASK_FILE_CONTENT_MAX];
    int status;
    int count = 0;
    int i = 0;
    int start = 0;

    if (count_out == 0 || tasks_out == 0 || max_tasks <= 0) {
        return 0;
    }

    *count_out = 0;
    task_file_path_fixed(path, sizeof(path));
    status = filesystem_cat(path, content, sizeof(content));
    if (status == FS_ERR_NOT_FOUND) {
        return 1;
    }
    if (status != FS_OK) {
        return 0;
    }

    while (1) {
        if (content[i] == '\n' || content[i] == '\0') {
            if (i > start && count < max_tasks) {
                char line[256];
                int line_len = i - start;
                char *fields[6];
                int field_count;
                int id = 0;
                int priority = 0;
                int status_value = 0;
                int j = 0;

                if (line_len >= (int)sizeof(line)) {
                    line_len = (int)sizeof(line) - 1;
                }
                for (j = 0; j < line_len; j++) {
                    line[j] = content[start + j];
                }
                line[line_len] = '\0';

                field_count = split_fields(line, fields, 6);
                if (field_count >= 6 && parse_int(fields[0], &id) && parse_int(fields[1], &priority) && parse_int(fields[2], &status_value)) {
                    tasks_out[count].id = id;
                    tasks_out[count].priority = priority;
                    tasks_out[count].status = status_value;
                    str_copy(tasks_out[count].title, fields[3], sizeof(tasks_out[count].title));
                    str_copy(tasks_out[count].due, fields[4], sizeof(tasks_out[count].due));
                    str_copy(tasks_out[count].description, fields[5], sizeof(tasks_out[count].description));
                    count++;
                }
            }
            if (content[i] == '\0') {
                break;
            }
            start = i + 1;
        }
        i++;
    }

    *count_out = count;
    return 1;
}

static int save_tasks(const task_record_t *tasks, int count) {
    char path[TASK_FILE_PATH_MAX];
    char content[TASK_FILE_CONTENT_MAX];
    unsigned int length = 0;
    int i = 0;

    task_file_path_fixed(path, sizeof(path));
    content[0] = '\0';

    for (i = 0; i < count; i++) {
        char title[64];
        char due[16];
        char description[96];

        sanitize_field(title, sizeof(title), tasks[i].title);
        sanitize_field(due, sizeof(due), tasks[i].due);
        sanitize_field(description, sizeof(description), tasks[i].description);

        if (!append_int(content, sizeof(content), &length, tasks[i].id) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_int(content, sizeof(content), &length, tasks[i].priority) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_int(content, sizeof(content), &length, tasks[i].status) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, title) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, due) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, description) ||
            !append_text(content, sizeof(content), &length, "\n")) {
            return 0;
        }
    }

    if (!ensure_task_file_exists()) {
        return 0;
    }
    return filesystem_write(path, content) == FS_OK;
}

const char *tasks_priority_name(int priority) {
    if (priority == TASK_PRIORITY_HIGH) {
        return "HIGH";
    }
    if (priority == TASK_PRIORITY_MEDIUM) {
        return "MEDIUM";
    }
    return "LOW";
}

const char *tasks_status_name(int status) {
    if (status == TASK_STATUS_COMPLETED) {
        return "COMPLETED";
    }
    return "PENDING";
}

int tasks_add(const char *title, const char *description, int priority, const char *due, int *task_id_out) {
    task_record_t tasks[TASK_MAX_RECORDS];
    int count = 0;
    int next_id = 1;
    int i = 0;

    if (!security_is_logged_in() || title == 0 || title[0] == '\0' || task_id_out == 0) {
        return 0;
    }
    if (priority < TASK_PRIORITY_LOW || priority > TASK_PRIORITY_HIGH) {
        return 0;
    }
    if (!load_tasks(tasks, TASK_MAX_RECORDS, &count)) {
        return 0;
    }
    if (count >= TASK_MAX_RECORDS) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (tasks[i].id >= next_id) {
            next_id = tasks[i].id + 1;
        }
    }

    tasks[count].id = next_id;
    tasks[count].priority = priority;
    tasks[count].status = TASK_STATUS_PENDING;
    str_copy(tasks[count].title, title, sizeof(tasks[count].title));
    str_copy(tasks[count].description, description ? description : "", sizeof(tasks[count].description));
    str_copy(tasks[count].due, due ? due : "", sizeof(tasks[count].due));

    if (!save_tasks(tasks, count + 1)) {
        return 0;
    }

    *task_id_out = next_id;
    return 1;
}

int tasks_mark_done(int task_id) {
    task_record_t tasks[TASK_MAX_RECORDS];
    int count = 0;
    int i = 0;

    if (!load_tasks(tasks, TASK_MAX_RECORDS, &count)) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (tasks[i].id == task_id) {
            tasks[i].status = TASK_STATUS_COMPLETED;
            if (!save_tasks(tasks, count)) {
                return -1;
            }
            return 1;
        }
    }

    return 0;
}

int tasks_delete(int task_id) {
    task_record_t tasks[TASK_MAX_RECORDS];
    task_record_t kept[TASK_MAX_RECORDS];
    int count = 0;
    int kept_count = 0;
    int i = 0;
    int removed = 0;

    if (!load_tasks(tasks, TASK_MAX_RECORDS, &count)) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (tasks[i].id == task_id) {
            removed = 1;
            continue;
        }
        kept[kept_count++] = tasks[i];
    }

    if (!removed) {
        return 0;
    }
    if (!save_tasks(kept, kept_count)) {
        return -1;
    }
    return 1;
}

int tasks_clear_completed(int *removed_out) {
    task_record_t tasks[TASK_MAX_RECORDS];
    task_record_t kept[TASK_MAX_RECORDS];
    int count = 0;
    int kept_count = 0;
    int removed_count = 0;
    int i = 0;

    if (!load_tasks(tasks, TASK_MAX_RECORDS, &count)) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (tasks[i].status == TASK_STATUS_COMPLETED) {
            removed_count++;
            continue;
        }
        kept[kept_count++] = tasks[i];
    }

    if (!save_tasks(kept, kept_count)) {
        return 0;
    }

    if (removed_out) {
        *removed_out = removed_count;
    }
    return 1;
}

int tasks_get(int task_id, task_record_t *task_out) {
    task_record_t tasks[TASK_MAX_RECORDS];
    int count = 0;
    int i = 0;

    if (task_out == 0) {
        return -1;
    }
    if (!load_tasks(tasks, TASK_MAX_RECORDS, &count)) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (tasks[i].id == task_id) {
            *task_out = tasks[i];
            return 1;
        }
    }

    return 0;
}

int tasks_list(task_record_t *tasks_out, int max_tasks, int *count_out) {
    return load_tasks(tasks_out, max_tasks, count_out);
}

void tasks_stats(int *pending_out, int *completed_out, int *total_out) {
    task_record_t tasks[TASK_MAX_RECORDS];
    int count = 0;
    int pending = 0;
    int completed = 0;
    int i = 0;

    if (!load_tasks(tasks, TASK_MAX_RECORDS, &count)) {
        if (pending_out) {
            *pending_out = 0;
        }
        if (completed_out) {
            *completed_out = 0;
        }
        if (total_out) {
            *total_out = 0;
        }
        return;
    }

    for (i = 0; i < count; i++) {
        if (tasks[i].status == TASK_STATUS_COMPLETED) {
            completed++;
        } else {
            pending++;
        }
    }

    if (pending_out) {
        *pending_out = pending;
    }
    if (completed_out) {
        *completed_out = completed;
    }
    if (total_out) {
        *total_out = count;
    }
}
