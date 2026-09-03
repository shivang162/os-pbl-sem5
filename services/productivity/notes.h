#ifndef STUDYOS_NOTES_H
#define STUDYOS_NOTES_H

typedef struct {
    int id;
    char title[64];
    char content[512];
} note_record_t;

int notes_add(const char *title, const char *content, int *note_id_out);
int notes_delete(int note_id);
int notes_get(int note_id, note_record_t *note_out);
int notes_list(note_record_t *notes_out, int max_notes, int *count_out);
int notes_count(void);

#endif
