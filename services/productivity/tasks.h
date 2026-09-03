#ifndef STUDYOS_TASKS_H
#define STUDYOS_TASKS_H

#define TASK_PRIORITY_LOW 1
#define TASK_PRIORITY_MEDIUM 2
#define TASK_PRIORITY_HIGH 3

#define TASK_STATUS_PENDING 0
#define TASK_STATUS_COMPLETED 1

typedef struct {
    int id;
    int priority;
    int status;
    char title[64];
    char description[96];
    char due[16];
} task_record_t;

int tasks_add(const char *title, const char *description, int priority, const char *due, int *task_id_out);
int tasks_mark_done(int task_id);
int tasks_delete(int task_id);
int tasks_clear_completed(int *removed_out);
int tasks_get(int task_id, task_record_t *task_out);
int tasks_list(task_record_t *tasks_out, int max_tasks, int *count_out);
void tasks_stats(int *pending_out, int *completed_out, int *total_out);

const char *tasks_priority_name(int priority);
const char *tasks_status_name(int status);

#endif
