#include "timetable.h"
#include "../../filesystem/filesystem.h"
#include "../../security/security.h"

#define TIMETABLE_FILE_PATH_MAX 64
#define TIMETABLE_FILE_CONTENT_MAX FS_CONTENT_MAX
#define TIMETABLE_MAX_RECORDS 48

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

static int ascii_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

static int str_equal_case(const char *a, const char *b) {
    unsigned int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (ascii_upper(a[i]) != ascii_upper(b[i])) {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
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
    int i;
    unsigned int number;

    if (value <= 0) {
        return append_text(buffer, buffer_size, length, "0");
    }

    number = (unsigned int)value;
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
    int value = 0;
    unsigned int i = 0;
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

static void sanitize_text(char *out, unsigned int out_size, const char *input) {
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

static void timetable_file_path(char *path_out, unsigned int path_size) {
    unsigned int len = 0;
    const char *prefix = "/home/";
    const char *user = security_current_username();
    const char *suffix = "/schedule.db";
    unsigned int i = 0;

    if (path_size == 0) {
        return;
    }

    while (prefix[i] != '\0' && len < path_size - 1) {
        path_out[len++] = prefix[i++];
    }

    i = 0;
    while (user[i] != '\0' && len < path_size - 1) {
        path_out[len++] = user[i++];
    }

    i = 0;
    while (suffix[i] != '\0' && len < path_size - 1) {
        path_out[len++] = suffix[i++];
    }

    path_out[len] = '\0';
}

static int ensure_timetable_file_exists(void) {
    char path[TIMETABLE_FILE_PATH_MAX];
    int status;
    timetable_file_path(path, sizeof(path));
    status = filesystem_touch(path);
    return status == FS_OK || status == FS_ERR_EXISTS;
}

static int compare_entries(const timetable_record_t *a, const timetable_record_t *b) {
    int i;
    if (a->day != b->day) {
        return a->day - b->day;
    }
    for (i = 0; i < 5; i++) {
        if (a->start[i] != b->start[i]) {
            return (int)a->start[i] - (int)b->start[i];
        }
    }
    return 0;
}

static void sort_entries(timetable_record_t *entries, int count) {
    int i;
    int j;
    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (compare_entries(&entries[j], &entries[i]) < 0) {
                timetable_record_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }
}

static int load_timetable(timetable_record_t *entries_out, int max_entries, int *count_out) {
    char path[TIMETABLE_FILE_PATH_MAX];
    char content[TIMETABLE_FILE_CONTENT_MAX];
    int status;
    int i = 0;
    int start = 0;
    int count = 0;

    if (entries_out == 0 || count_out == 0 || max_entries <= 0) {
        return 0;
    }

    *count_out = 0;
    timetable_file_path(path, sizeof(path));
    status = filesystem_cat(path, content, sizeof(content));
    if (status == FS_ERR_NOT_FOUND) {
        return 1;
    }
    if (status != FS_OK) {
        return 0;
    }

    while (1) {
        if (content[i] == '\n' || content[i] == '\0') {
            if (i > start && count < max_entries) {
                char line[256];
                int line_len = i - start;
                int j;
                char *fields[7];
                int field_count;
                int id;
                int day;

                if (line_len >= (int)sizeof(line)) {
                    line_len = (int)sizeof(line) - 1;
                }
                for (j = 0; j < line_len; j++) {
                    line[j] = content[start + j];
                }
                line[line_len] = '\0';

                field_count = split_fields(line, fields, 7);
                if (field_count >= 7 && parse_int(fields[0], &id) && parse_int(fields[1], &day)) {
                    entries_out[count].id = id;
                    entries_out[count].day = day;
                    str_copy(entries_out[count].start, fields[2], sizeof(entries_out[count].start));
                    str_copy(entries_out[count].end, fields[3], sizeof(entries_out[count].end));
                    str_copy(entries_out[count].subject, fields[4], sizeof(entries_out[count].subject));
                    str_copy(entries_out[count].room, fields[5], sizeof(entries_out[count].room));
                    str_copy(entries_out[count].teacher, fields[6], sizeof(entries_out[count].teacher));
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

    sort_entries(entries_out, count);
    *count_out = count;
    return 1;
}

static int save_timetable(timetable_record_t *entries, int count) {
    char path[TIMETABLE_FILE_PATH_MAX];
    char content[TIMETABLE_FILE_CONTENT_MAX];
    unsigned int length = 0;
    int i;

    content[0] = '\0';
    sort_entries(entries, count);
    timetable_file_path(path, sizeof(path));

    for (i = 0; i < count; i++) {
        char subject[64];
        char room[32];
        char teacher[32];

        sanitize_text(subject, sizeof(subject), entries[i].subject);
        sanitize_text(room, sizeof(room), entries[i].room);
        sanitize_text(teacher, sizeof(teacher), entries[i].teacher);

        if (!append_int(content, sizeof(content), &length, entries[i].id) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_int(content, sizeof(content), &length, entries[i].day) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, entries[i].start) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, entries[i].end) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, subject) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, room) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, teacher) ||
            !append_text(content, sizeof(content), &length, "\n")) {
            return 0;
        }
    }

    if (!ensure_timetable_file_exists()) {
        return 0;
    }

    return filesystem_write(path, content) == FS_OK;
}

int timetable_parse_day(const char *text, int *day_out) {
    if (text == 0 || day_out == 0) {
        return 0;
    }
    if (str_equal_case(text, "Monday")) {
        *day_out = 0;
        return 1;
    }
    if (str_equal_case(text, "Tuesday")) {
        *day_out = 1;
        return 1;
    }
    if (str_equal_case(text, "Wednesday")) {
        *day_out = 2;
        return 1;
    }
    if (str_equal_case(text, "Thursday")) {
        *day_out = 3;
        return 1;
    }
    if (str_equal_case(text, "Friday")) {
        *day_out = 4;
        return 1;
    }
    if (str_equal_case(text, "Saturday")) {
        *day_out = 5;
        return 1;
    }
    if (str_equal_case(text, "Sunday")) {
        *day_out = 6;
        return 1;
    }
    return 0;
}

const char *timetable_day_name(int day) {
    if (day == 0) {
        return "MONDAY";
    }
    if (day == 1) {
        return "TUESDAY";
    }
    if (day == 2) {
        return "WEDNESDAY";
    }
    if (day == 3) {
        return "THURSDAY";
    }
    if (day == 4) {
        return "FRIDAY";
    }
    if (day == 5) {
        return "SATURDAY";
    }
    return "SUNDAY";
}

int timetable_time_is_valid(const char *text) {
    int hh;
    int mm;
    if (text == 0 || text[0] == '\0') {
        return 0;
    }
    if (text[0] < '0' || text[0] > '9' ||
        text[1] < '0' || text[1] > '9' ||
        text[2] != ':' ||
        text[3] < '0' || text[3] > '9' ||
        text[4] < '0' || text[4] > '9' ||
        text[5] != '\0') {
        return 0;
    }

    hh = (text[0] - '0') * 10 + (text[1] - '0');
    mm = (text[3] - '0') * 10 + (text[4] - '0');

    return hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59;
}

int timetable_add(int day, const char *start, const char *end, const char *subject, const char *room, const char *teacher, int *entry_id_out) {
    timetable_record_t entries[TIMETABLE_MAX_RECORDS];
    int count = 0;
    int next_id = 1;
    int i;

    if (!security_is_logged_in() || entry_id_out == 0 || subject == 0 || subject[0] == '\0') {
        return 0;
    }
    if (day < 0 || day > 6 || !timetable_time_is_valid(start) || !timetable_time_is_valid(end)) {
        return 0;
    }
    if (!load_timetable(entries, TIMETABLE_MAX_RECORDS, &count)) {
        return 0;
    }
    if (count >= TIMETABLE_MAX_RECORDS) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (entries[i].id >= next_id) {
            next_id = entries[i].id + 1;
        }
    }

    entries[count].id = next_id;
    entries[count].day = day;
    str_copy(entries[count].start, start, sizeof(entries[count].start));
    str_copy(entries[count].end, end, sizeof(entries[count].end));
    str_copy(entries[count].subject, subject, sizeof(entries[count].subject));
    str_copy(entries[count].room, room ? room : "", sizeof(entries[count].room));
    str_copy(entries[count].teacher, teacher ? teacher : "", sizeof(entries[count].teacher));

    if (!save_timetable(entries, count + 1)) {
        return 0;
    }

    *entry_id_out = next_id;
    return 1;
}

int timetable_delete(int entry_id) {
    timetable_record_t entries[TIMETABLE_MAX_RECORDS];
    timetable_record_t kept[TIMETABLE_MAX_RECORDS];
    int count = 0;
    int kept_count = 0;
    int i;
    int removed = 0;

    if (!load_timetable(entries, TIMETABLE_MAX_RECORDS, &count)) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (entries[i].id == entry_id) {
            removed = 1;
            continue;
        }
        kept[kept_count++] = entries[i];
    }

    if (!removed) {
        return 0;
    }
    if (!save_timetable(kept, kept_count)) {
        return -1;
    }
    return 1;
}

int timetable_list(timetable_record_t *entries_out, int max_entries, int *count_out) {
    return load_timetable(entries_out, max_entries, count_out);
}

int timetable_count(void) {
    timetable_record_t entries[TIMETABLE_MAX_RECORDS];
    int count = 0;
    if (!load_timetable(entries, TIMETABLE_MAX_RECORDS, &count)) {
        return 0;
    }
    return count;
}

int timetable_next_class_for_day(int day, timetable_record_t *entry_out) {
    timetable_record_t entries[TIMETABLE_MAX_RECORDS];
    int count = 0;
    int i;

    if (entry_out == 0) {
        return 0;
    }

    if (!load_timetable(entries, TIMETABLE_MAX_RECORDS, &count)) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (entries[i].day == day) {
            *entry_out = entries[i];
            return 1;
        }
    }

    return 0;
}

int timetable_classes_for_day(int day) {
    timetable_record_t entries[TIMETABLE_MAX_RECORDS];
    int count = 0;
    int matches = 0;
    int i;

    if (!load_timetable(entries, TIMETABLE_MAX_RECORDS, &count)) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (entries[i].day == day) {
            matches++;
        }
    }
    return matches;
}
