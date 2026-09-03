#include "notes.h"
#include "../../filesystem/filesystem.h"
#include "../../security/security.h"

#define NOTE_FILE_PATH_MAX 64
#define NOTE_FILE_CONTENT_MAX FS_CONTENT_MAX
#define NOTE_MAX_RECORDS 16

static unsigned int str_len(const char *text) {
    unsigned int length = 0;
    while (text[length] != '\0') {
        length++;
    }
    return length;
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

static void sanitize_simple(char *out, unsigned int out_size, const char *input) {
    unsigned int i = 0;
    unsigned int j = 0;
    if (out_size == 0) {
        return;
    }

    while (input[i] != '\0' && j < out_size - 1) {
        if (input[i] == '|' || input[i] == '\r') {
            out[j++] = ' ';
        } else {
            out[j++] = input[i];
        }
        i++;
    }
    out[j] = '\0';
}

static void encode_note_content(char *out, unsigned int out_size, const char *input) {
    unsigned int i = 0;
    unsigned int j = 0;
    if (out_size == 0) {
        return;
    }

    while (input[i] != '\0' && j < out_size - 1) {
        if (input[i] == '|') {
            if (j + 2 >= out_size) {
                break;
            }
            out[j++] = '\\';
            out[j++] = '|';
        } else if (input[i] == '\n') {
            if (j + 2 >= out_size) {
                break;
            }
            out[j++] = '\\';
            out[j++] = 'n';
        } else if (input[i] == '\\') {
            if (j + 2 >= out_size) {
                break;
            }
            out[j++] = '\\';
            out[j++] = '\\';
        } else {
            out[j++] = input[i];
        }
        i++;
    }
    out[j] = '\0';
}

static void decode_note_content(char *out, unsigned int out_size, const char *input) {
    unsigned int i = 0;
    unsigned int j = 0;
    if (out_size == 0) {
        return;
    }

    while (input[i] != '\0' && j < out_size - 1) {
        if (input[i] == '\\' && input[i + 1] != '\0') {
            if (input[i + 1] == 'n') {
                out[j++] = '\n';
                i += 2;
                continue;
            }
            out[j++] = input[i + 1];
            i += 2;
            continue;
        }
        out[j++] = input[i++];
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
        if (line[i] == '|' && (i == 0 || line[i - 1] != '\\')) {
            line[i] = '\0';
            fields[count++] = &line[i + 1];
        }
        i++;
    }
    return count;
}

static void notes_file_path(char *path_out, unsigned int path_size) {
    unsigned int len = 0;
    const char *prefix = "/home/";
    const char *user = security_current_username();
    const char *suffix = "/notes.db";
    unsigned int i = 0;

    if (path_size == 0) {
        return;
    }

    path_out[0] = '\0';
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

static int ensure_notes_file_exists(void) {
    char path[NOTE_FILE_PATH_MAX];
    int status;
    notes_file_path(path, sizeof(path));
    status = filesystem_touch(path);
    return status == FS_OK || status == FS_ERR_EXISTS;
}

static int load_notes(note_record_t *notes_out, int max_notes, int *count_out) {
    char path[NOTE_FILE_PATH_MAX];
    char content[NOTE_FILE_CONTENT_MAX];
    int status;
    int i = 0;
    int start = 0;
    int count = 0;

    if (notes_out == 0 || count_out == 0 || max_notes <= 0) {
        return 0;
    }

    *count_out = 0;
    notes_file_path(path, sizeof(path));
    status = filesystem_cat(path, content, sizeof(content));
    if (status == FS_ERR_NOT_FOUND) {
        return 1;
    }
    if (status != FS_OK) {
        return 0;
    }

    while (1) {
        if (content[i] == '\n' || content[i] == '\0') {
            if (i > start && count < max_notes) {
                char line[768];
                int line_len = i - start;
                int j;
                char *fields[3];
                int field_count;
                int id;

                if (line_len >= (int)sizeof(line)) {
                    line_len = (int)sizeof(line) - 1;
                }
                for (j = 0; j < line_len; j++) {
                    line[j] = content[start + j];
                }
                line[line_len] = '\0';

                field_count = split_fields(line, fields, 3);
                if (field_count >= 3 && parse_int(fields[0], &id)) {
                    notes_out[count].id = id;
                    sanitize_simple(notes_out[count].title, sizeof(notes_out[count].title), fields[1]);
                    decode_note_content(notes_out[count].content, sizeof(notes_out[count].content), fields[2]);
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

static int save_notes(const note_record_t *notes, int count) {
    char path[NOTE_FILE_PATH_MAX];
    char content[NOTE_FILE_CONTENT_MAX];
    unsigned int length = 0;
    int i;

    notes_file_path(path, sizeof(path));
    content[0] = '\0';

    for (i = 0; i < count; i++) {
        char title[64];
        char encoded[768];
        sanitize_simple(title, sizeof(title), notes[i].title);
        encode_note_content(encoded, sizeof(encoded), notes[i].content);

        if (!append_int(content, sizeof(content), &length, notes[i].id) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, title) ||
            !append_text(content, sizeof(content), &length, "|") ||
            !append_text(content, sizeof(content), &length, encoded) ||
            !append_text(content, sizeof(content), &length, "\n")) {
            return 0;
        }
    }

    if (!ensure_notes_file_exists()) {
        return 0;
    }
    return filesystem_write(path, content) == FS_OK;
}

int notes_add(const char *title, const char *content, int *note_id_out) {
    note_record_t notes[NOTE_MAX_RECORDS];
    int count = 0;
    int next_id = 1;
    int i;

    if (!security_is_logged_in() || title == 0 || title[0] == '\0' || content == 0 || content[0] == '\0' || note_id_out == 0) {
        return 0;
    }

    if (!load_notes(notes, NOTE_MAX_RECORDS, &count)) {
        return 0;
    }
    if (count >= NOTE_MAX_RECORDS) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (notes[i].id >= next_id) {
            next_id = notes[i].id + 1;
        }
    }

    notes[count].id = next_id;
    str_copy(notes[count].title, title, sizeof(notes[count].title));
    str_copy(notes[count].content, content, sizeof(notes[count].content));

    if (!save_notes(notes, count + 1)) {
        return 0;
    }

    *note_id_out = next_id;
    return 1;
}

int notes_delete(int note_id) {
    note_record_t notes[NOTE_MAX_RECORDS];
    note_record_t kept[NOTE_MAX_RECORDS];
    int count = 0;
    int kept_count = 0;
    int i;
    int removed = 0;

    if (!load_notes(notes, NOTE_MAX_RECORDS, &count)) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (notes[i].id == note_id) {
            removed = 1;
            continue;
        }
        kept[kept_count++] = notes[i];
    }

    if (!removed) {
        return 0;
    }

    if (!save_notes(kept, kept_count)) {
        return -1;
    }
    return 1;
}

int notes_get(int note_id, note_record_t *note_out) {
    note_record_t notes[NOTE_MAX_RECORDS];
    int count = 0;
    int i;

    if (note_out == 0) {
        return -1;
    }

    if (!load_notes(notes, NOTE_MAX_RECORDS, &count)) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        if (notes[i].id == note_id) {
            *note_out = notes[i];
            return 1;
        }
    }
    return 0;
}

int notes_list(note_record_t *notes_out, int max_notes, int *count_out) {
    return load_notes(notes_out, max_notes, count_out);
}

int notes_count(void) {
    note_record_t notes[NOTE_MAX_RECORDS];
    int count = 0;
    if (!load_notes(notes, NOTE_MAX_RECORDS, &count)) {
        return 0;
    }
    return count;
}
