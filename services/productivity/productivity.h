#ifndef STUDYOS_PRODUCTIVITY_H
#define STUDYOS_PRODUCTIVITY_H

void productivity_init(void);

void productivity_task_created(int task_id);
void productivity_task_completed(int task_id);
void productivity_note_created(int note_id);
void productivity_study_session_completed(void);

void productivity_command_task(const char *args);
void productivity_command_tasks_shortcut(void);
void productivity_command_note(const char *args);
void productivity_command_notes_shortcut(void);
void productivity_command_schedule(const char *args);
void productivity_command_timetable_shortcut(void);
void productivity_command_dashboard(void);
void productivity_command_overview(void);
void productivity_show_dashboard_if_enabled(void);

#endif
