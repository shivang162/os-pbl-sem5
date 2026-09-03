#ifndef STUDYOS_TIMETABLE_H
#define STUDYOS_TIMETABLE_H

typedef struct {
    int id;
    int day;
    char start[6];
    char end[6];
    char subject[64];
    char room[32];
    char teacher[32];
} timetable_record_t;

int timetable_parse_day(const char *text, int *day_out);
const char *timetable_day_name(int day);

int timetable_add(int day, const char *start, const char *end, const char *subject, const char *room, const char *teacher, int *entry_id_out);
int timetable_delete(int entry_id);
int timetable_list(timetable_record_t *entries_out, int max_entries, int *count_out);
int timetable_count(void);
int timetable_next_class_for_day(int day, timetable_record_t *entry_out);
int timetable_classes_for_day(int day);
int timetable_time_is_valid(const char *text);

#endif
