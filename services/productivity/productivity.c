#include "productivity.h"
#include "tasks.h"
#include "notes.h"
#include "timetable.h"
#include "../../drivers/keyboard.h"
#include "../../kernel/terminal.h"
#include "../../security/security.h"

#define PRODUCTIVITY_MAX_TASKS 32
#define PRODUCTIVITY_MAX_NOTES 16
#define PRODUCTIVITY_MAX_SCHEDULE 48

static int is_space(char c) {
    return c == ' ' || c == '\t';
}

static int is_printable(char c) {
    return c >= 32 && c <= 126;
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

static int str_len(const char *text) {
    int len = 0;
    while (text[len] != '\0') {
        len++;
    }
    return len;
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

static void skip_spaces(const char **cursor) {
    while (is_space(**cursor)) {
        (*cursor)++;
    }
}

static int read_token(const char **cursor, char *token, unsigned int size) {
    unsigned int i = 0;
    skip_spaces(cursor);

    if (**cursor == '\0' || size == 0) {
        return 0;
    }

    while (**cursor != '\0' && !is_space(**cursor) && i < size - 1) {
        token[i++] = **cursor;
        (*cursor)++;
    }
    token[i] = '\0';

    while (**cursor != '\0' && !is_space(**cursor)) {
        (*cursor)++;
    }
    return i > 0;
}

static void read_input(char *buffer, unsigned int size, int mask) {
    unsigned int length = 0;

    if (size == 0) {
        return;
    }

    buffer[0] = '\0';

    while (1) {
        char key = keyboard_read_char();

        if (key == '\n') {
            terminal_put_char('\n');
            break;
        }

        if (key == '\b') {
            if (length > 0) {
                length--;
                buffer[length] = '\0';
                terminal_backspace();
            }
            continue;
        }

        if (!is_printable(key)) {
            continue;
        }

        if (length < size - 1) {
            buffer[length++] = key;
            buffer[length] = '\0';
            terminal_put_char(mask ? '*' : key);
        }
    }
}

static int parse_id(const char *text, int *value_out) {
    int value = 0;
    int i = 0;

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

    if (value <= 0) {
        return 0;
    }

    *value_out = value;
    return 1;
}

static void write_int(int value) {
    char digits[16];
    int count = 0;
    int i;
    unsigned int number;

    if (value == 0) {
        terminal_write("0");
        return;
    }

    if (value < 0) {
        terminal_write("-");
        number = (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }

    while (number > 0 && count < (int)sizeof(digits)) {
        digits[count++] = (char)('0' + (number % 10));
        number /= 10;
    }

    for (i = count - 1; i >= 0; i--) {
        char c[2];
        c[0] = digits[i];
        c[1] = '\0';
        terminal_write(c);
    }
}

static void write_padding(const char *text, int width) {
    int i = 0;
    int len = str_len(text);
    terminal_write(text);
    for (i = len; i < width; i++) {
        terminal_write(" ");
    }
}

static int parse_priority(const char *text, int *priority_out) {
    if (str_equal(text, "1")) {
        *priority_out = TASK_PRIORITY_LOW;
        return 1;
    }
    if (str_equal(text, "2")) {
        *priority_out = TASK_PRIORITY_MEDIUM;
        return 1;
    }
    if (str_equal(text, "3")) {
        *priority_out = TASK_PRIORITY_HIGH;
        return 1;
    }
    return 0;
}

static int confirm_yes(const char *question) {
    char answer[4];
    terminal_write("\n");
    terminal_write(question);
    terminal_write("\n\n[Y/N]: ");
    read_input(answer, sizeof(answer), 0);
    return str_equal(answer, "Y") || str_equal(answer, "y");
}

static void task_usage(void) {
    terminal_write("\nUsage:\n");
    terminal_write("task add\n");
    terminal_write("task list\n");
    terminal_write("task view <id>\n");
    terminal_write("task done <id>\n");
    terminal_write("task delete <id>\n");
    terminal_write("task clear\n\n");
}

static void note_usage(void) {
    terminal_write("\nUsage:\n");
    terminal_write("note add\n");
    terminal_write("note list\n");
    terminal_write("note view <id>\n");
    terminal_write("note delete <id>\n\n");
}

static void schedule_usage(void) {
    terminal_write("\nUsage:\n");
    terminal_write("schedule add\n");
    terminal_write("schedule list\n");
    terminal_write("schedule today\n");
    terminal_write("schedule delete <id>\n\n");
}

static void task_add_interactive(void) {
    char title[64];
    char description[96];
    char due[16];
    char priority_text[4];
    int priority;
    int task_id;

    terminal_write("\nTask title: ");
    read_input(title, sizeof(title), 0);
    if (title[0] == '\0') {
        terminal_write("\nTask title cannot be empty.\n\n");
        return;
    }

    terminal_write("Description (optional): ");
    read_input(description, sizeof(description), 0);

    terminal_write("Priority:\n");
    terminal_write("1. Low\n");
    terminal_write("2. Medium\n");
    terminal_write("3. High\n\n");
    terminal_write("Select: ");
    read_input(priority_text, sizeof(priority_text), 0);

    if (!parse_priority(priority_text, &priority)) {
        terminal_write("\nInvalid priority.\n");
        terminal_write("Choose:\n1. Low\n2. Medium\n3. High\n\n");
        return;
    }

    terminal_write("Due date (optional): ");
    read_input(due, sizeof(due), 0);

    if (!tasks_add(title, description, priority, due, &task_id)) {
        terminal_write("\nFailed to create task.\n\n");
        return;
    }

    productivity_task_created(task_id);

    terminal_write("\nTask created successfully.\n\nTask ID: ");
    write_int(task_id);
    terminal_write("\n\n");
}

static void task_list_all(void) {
    task_record_t tasks[PRODUCTIVITY_MAX_TASKS];
    int count = 0;
    int i;
    int pending;
    int completed;
    int total;

    if (!tasks_list(tasks, PRODUCTIVITY_MAX_TASKS, &count)) {
        terminal_write("\nUnable to load tasks.\n\n");
        return;
    }

    terminal_write("\n========================================\n");
    terminal_write("             MY TASKS\n");
    terminal_write("========================================\n\n");
    terminal_write("ID   PRIORITY   STATUS       TASK\n\n");

    for (i = 0; i < count; i++) {
        const char *status = tasks[i].status == TASK_STATUS_COMPLETED ? "[✓]" : "[ ]";
        write_int(tasks[i].id);
        if (tasks[i].id < 10) {
            terminal_write("    ");
        } else {
            terminal_write("   ");
        }
        write_padding(tasks_priority_name(tasks[i].priority), 11);
        write_padding(status, 13);
        terminal_write(tasks[i].title);
        terminal_write("\n");
    }

    if (count == 0) {
        terminal_write("No tasks yet.\n");
    }

    tasks_stats(&pending, &completed, &total);
    terminal_write("\n========================================\n");
    terminal_write("Completed: ");
    write_int(completed);
    terminal_write("\nPending:   ");
    write_int(pending);
    terminal_write("\n========================================\n\n");
}

static void task_view_one(int task_id) {
    task_record_t task;
    int status = tasks_get(task_id, &task);

    if (status == 0) {
        terminal_write("\nTask not found.\n\n");
        return;
    }
    if (status < 0) {
        terminal_write("\nUnable to load task.\n\n");
        return;
    }

    terminal_write("\n================================\n");
    terminal_write("Task #");
    write_int(task.id);
    terminal_write("\n================================\n\n");

    terminal_write("Title:\n");
    terminal_write(task.title);
    terminal_write("\n\nPriority:\n");
    terminal_write(tasks_priority_name(task.priority));
    terminal_write("\n\nStatus:\n");
    terminal_write(tasks_status_name(task.status));
    terminal_write("\n\nDue:\n");
    if (task.due[0] == '\0') {
        terminal_write("Not set");
    } else {
        terminal_write(task.due);
    }

    if (task.description[0] != '\0') {
        terminal_write("\n\nDescription:\n");
        terminal_write(task.description);
    }

    terminal_write("\n\n================================\n\n");
}

static void task_mark_done_one(int task_id) {
    task_record_t task;
    int lookup = tasks_get(task_id, &task);
    int status;

    if (lookup == 0) {
        terminal_write("\nTask not found.\n\n");
        return;
    }
    if (lookup < 0) {
        terminal_write("\nUnable to load task.\n\n");
        return;
    }

    status = tasks_mark_done(task_id);
    if (status <= 0) {
        terminal_write("\nUnable to complete task.\n\n");
        return;
    }

    productivity_task_completed(task_id);

    terminal_write("\nTask completed!\n\n✓ ");
    terminal_write(task.title);
    terminal_write("\n\n");
}

static void task_delete_one(int task_id) {
    int status;

    terminal_write("\nDelete task #");
    write_int(task_id);
    terminal_write("?");
    if (!confirm_yes("")) {
        terminal_write("\nTask deletion cancelled.\n\n");
        return;
    }

    status = tasks_delete(task_id);
    if (status == 0) {
        terminal_write("\nTask not found.\n\n");
        return;
    }
    if (status < 0) {
        terminal_write("\nUnable to delete task.\n\n");
        return;
    }

    terminal_write("\nTask deleted successfully.\n\n");
}

static void task_clear_completed_all(void) {
    int removed;
    if (!confirm_yes("Remove all completed tasks?")) {
        terminal_write("\nNo tasks cleared.\n\n");
        return;
    }

    if (!tasks_clear_completed(&removed)) {
        terminal_write("\nUnable to clear completed tasks.\n\n");
        return;
    }

    terminal_write("\nCompleted tasks removed: ");
    write_int(removed);
    terminal_write("\n\n");
}

static void note_add_interactive(void) {
    char title[64];
    char line[96];
    char content[512];
    int length = 0;
    int note_id;

    terminal_write("\nNote title: ");
    read_input(title, sizeof(title), 0);
    if (title[0] == '\0') {
        terminal_write("\nNote title cannot be empty.\n\n");
        return;
    }

    terminal_write("\nEnter note content.\nType END on a new line when finished.\n\n");

    content[0] = '\0';
    while (1) {
        int i = 0;
        terminal_write("> ");
        read_input(line, sizeof(line), 0);
        if (str_equal(line, "END")) {
            break;
        }

        if (line[0] == '\0') {
            if (length < (int)sizeof(content) - 2) {
                content[length++] = '\n';
                content[length] = '\0';
            }
            continue;
        }

        while (line[i] != '\0') {
            if (length >= (int)sizeof(content) - 2) {
                terminal_write("\nNote content is too long.\n\n");
                return;
            }
            content[length++] = line[i++];
        }
        content[length++] = '\n';
        content[length] = '\0';
    }

    if (content[0] == '\0') {
        terminal_write("\nNote cannot be empty.\n\n");
        return;
    }

    if (!notes_add(title, content, &note_id)) {
        terminal_write("\nFailed to save note.\n\n");
        return;
    }

    productivity_note_created(note_id);

    terminal_write("\nNote saved successfully.\n\nNote ID: ");
    write_int(note_id);
    terminal_write("\n\n");
}

static void note_list_all(void) {
    note_record_t notes[PRODUCTIVITY_MAX_NOTES];
    int count = 0;
    int i;

    if (!notes_list(notes, PRODUCTIVITY_MAX_NOTES, &count)) {
        terminal_write("\nUnable to load notes.\n\n");
        return;
    }

    terminal_write("\n========================================\n");
    terminal_write("              MY NOTES\n");
    terminal_write("========================================\n\n");
    terminal_write("ID     TITLE\n\n");

    for (i = 0; i < count; i++) {
        write_int(notes[i].id);
        if (notes[i].id < 10) {
            terminal_write("      ");
        } else {
            terminal_write("     ");
        }
        terminal_write(notes[i].title);
        terminal_write("\n");
    }

    if (count == 0) {
        terminal_write("No notes yet.\n");
    }

    terminal_write("\n========================================\n");
    terminal_write("Total Notes: ");
    write_int(count);
    terminal_write("\n========================================\n\n");
}

static void note_view_one(int note_id) {
    note_record_t note;
    int status = notes_get(note_id, &note);

    if (status == 0) {
        terminal_write("\nNote not found.\n\n");
        return;
    }
    if (status < 0) {
        terminal_write("\nUnable to load note.\n\n");
        return;
    }

    terminal_write("\n========================================\n");
    terminal_write("      ");
    terminal_write(note.title);
    terminal_write("\n========================================\n\n");
    terminal_write(note.content);
    terminal_write("\n========================================\n\n");
}

static void note_delete_one(int note_id) {
    int status;

    terminal_write("\nDelete note #");
    write_int(note_id);
    terminal_write("?");
    if (!confirm_yes("")) {
        terminal_write("\nNote deletion cancelled.\n\n");
        return;
    }

    status = notes_delete(note_id);
    if (status == 0) {
        terminal_write("\nNote not found.\n\n");
        return;
    }
    if (status < 0) {
        terminal_write("\nUnable to delete note.\n\n");
        return;
    }

    terminal_write("\nNote deleted successfully.\n\n");
}

static void schedule_add_interactive(void) {
    char day_text[16];
    char start[8];
    char end[8];
    char subject[64];
    char room[32];
    char teacher[32];
    int day;
    int entry_id;

    terminal_write("\nDay: ");
    read_input(day_text, sizeof(day_text), 0);
    if (!timetable_parse_day(day_text, &day)) {
        terminal_write("\nInvalid day.\nUse Monday-Sunday.\n\n");
        return;
    }

    terminal_write("\nStart time (HH:MM): ");
    read_input(start, sizeof(start), 0);
    terminal_write("End time (HH:MM): ");
    read_input(end, sizeof(end), 0);

    if (!timetable_time_is_valid(start) || !timetable_time_is_valid(end)) {
        terminal_write("\nInvalid time format. Use HH:MM.\n\n");
        return;
    }

    terminal_write("\nSubject: ");
    read_input(subject, sizeof(subject), 0);
    if (subject[0] == '\0') {
        terminal_write("\nSubject cannot be empty.\n\n");
        return;
    }

    terminal_write("\nRoom (optional): ");
    read_input(room, sizeof(room), 0);
    terminal_write("Teacher (optional): ");
    read_input(teacher, sizeof(teacher), 0);

    if (!timetable_add(day, start, end, subject, room, teacher, &entry_id)) {
        terminal_write("\nFailed to add schedule entry.\n\n");
        return;
    }

    terminal_write("\nSchedule added successfully.\n\nEntry ID: ");
    write_int(entry_id);
    terminal_write("\n\n");
}

static void schedule_list_all(void) {
    timetable_record_t entries[PRODUCTIVITY_MAX_SCHEDULE];
    int count = 0;
    int i;
    int current_day = -1;

    if (!timetable_list(entries, PRODUCTIVITY_MAX_SCHEDULE, &count)) {
        terminal_write("\nUnable to load timetable.\n\n");
        return;
    }

    terminal_write("\n====================================================\n");
    terminal_write("                 WEEKLY TIMETABLE\n");
    terminal_write("====================================================\n\n");

    for (i = 0; i < count; i++) {
        if (entries[i].day != current_day) {
            current_day = entries[i].day;
            terminal_write(timetable_day_name(entries[i].day));
            terminal_write("\n");
        }
        terminal_write(entries[i].start);
        terminal_write(" - ");
        terminal_write(entries[i].end);
        terminal_write("    ");
        write_padding(entries[i].subject, 24);
        if (entries[i].room[0] != '\0') {
            terminal_write(entries[i].room);
        }
        terminal_write("\n");
    }

    if (count == 0) {
        terminal_write("No timetable entries yet.\n");
    }

    terminal_write("\n====================================================\n\n");
}

static void schedule_today_view(void) {
    terminal_write("\nCurrent date unavailable.\n\nUse:\nschedule list\n\n");
}

static void schedule_delete_one(int entry_id) {
    int status;
    terminal_write("\nDelete schedule entry #");
    write_int(entry_id);
    terminal_write("?");
    if (!confirm_yes("")) {
        terminal_write("\nDelete cancelled.\n\n");
        return;
    }

    status = timetable_delete(entry_id);
    if (status == 0) {
        terminal_write("\nSchedule entry not found.\n\n");
        return;
    }
    if (status < 0) {
        terminal_write("\nUnable to delete schedule entry.\n\n");
        return;
    }

    terminal_write("\nSchedule deleted successfully.\n\n");
}

void productivity_init(void) {
}

void productivity_task_created(int task_id) {
    (void)task_id;
}

void productivity_task_completed(int task_id) {
    (void)task_id;
}

void productivity_note_created(int note_id) {
    (void)note_id;
}

void productivity_study_session_completed(void) {
}

void productivity_command_task(const char *args) {
    char action[16];
    char id_text[16];
    int id;

    if (!read_token(&args, action, sizeof(action))) {
        task_usage();
        return;
    }

    if (str_equal(action, "add")) {
        task_add_interactive();
        return;
    }
    if (str_equal(action, "list")) {
        task_list_all();
        return;
    }
    if (str_equal(action, "view")) {
        if (!read_token(&args, id_text, sizeof(id_text)) || !parse_id(id_text, &id)) {
            terminal_write("\nInvalid task ID.\n\n");
            return;
        }
        task_view_one(id);
        return;
    }
    if (str_equal(action, "done")) {
        if (!read_token(&args, id_text, sizeof(id_text)) || !parse_id(id_text, &id)) {
            terminal_write("\nInvalid task ID.\n\n");
            return;
        }
        task_mark_done_one(id);
        return;
    }
    if (str_equal(action, "delete")) {
        if (!read_token(&args, id_text, sizeof(id_text)) || !parse_id(id_text, &id)) {
            terminal_write("\nInvalid task ID.\n\n");
            return;
        }
        task_delete_one(id);
        return;
    }
    if (str_equal(action, "clear")) {
        task_clear_completed_all();
        return;
    }

    task_usage();
}

void productivity_command_tasks_shortcut(void) {
    task_list_all();
}

void productivity_command_note(const char *args) {
    char action[16];
    char id_text[16];
    int id;

    if (!read_token(&args, action, sizeof(action))) {
        note_usage();
        return;
    }

    if (str_equal(action, "add")) {
        note_add_interactive();
        return;
    }
    if (str_equal(action, "list")) {
        note_list_all();
        return;
    }
    if (str_equal(action, "view")) {
        if (!read_token(&args, id_text, sizeof(id_text)) || !parse_id(id_text, &id)) {
            terminal_write("\nInvalid note ID.\n\n");
            return;
        }
        note_view_one(id);
        return;
    }
    if (str_equal(action, "delete")) {
        if (!read_token(&args, id_text, sizeof(id_text)) || !parse_id(id_text, &id)) {
            terminal_write("\nInvalid note ID.\n\n");
            return;
        }
        note_delete_one(id);
        return;
    }

    note_usage();
}

void productivity_command_notes_shortcut(void) {
    note_list_all();
}

void productivity_command_schedule(const char *args) {
    char action[16];
    char id_text[16];
    int id;

    if (!read_token(&args, action, sizeof(action))) {
        schedule_usage();
        return;
    }

    if (str_equal(action, "add")) {
        schedule_add_interactive();
        return;
    }
    if (str_equal(action, "list")) {
        schedule_list_all();
        return;
    }
    if (str_equal(action, "today")) {
        schedule_today_view();
        return;
    }
    if (str_equal(action, "delete")) {
        if (!read_token(&args, id_text, sizeof(id_text)) || !parse_id(id_text, &id)) {
            terminal_write("\nInvalid schedule ID.\n\n");
            return;
        }
        schedule_delete_one(id);
        return;
    }

    schedule_usage();
}

void productivity_command_timetable_shortcut(void) {
    schedule_list_all();
}

void productivity_command_dashboard(void) {
    int pending;
    int completed;
    int total_tasks;
    int total_notes;
    int total_schedule;

    tasks_stats(&pending, &completed, &total_tasks);
    total_notes = notes_count();
    total_schedule = timetable_count();

    terminal_write("\n==========================================\n");
    terminal_write("              STUDYOS                     \n");
    terminal_write("          STUDENT DASHBOARD               \n");
    terminal_write("==========================================\n\n");

    terminal_write("Welcome, ");
    terminal_write(security_current_username());
    terminal_write("!\n\n");

    terminal_write("TODAY\n");
    terminal_write("------------------------------------------\n");
    terminal_write("Pending Tasks: ");
    write_int(pending);
    terminal_write("\nCompleted Tasks: ");
    write_int(completed);
    terminal_write("\nNotes: ");
    write_int(total_notes);
    terminal_write("\nClasses Today: unavailable\n\n");

    terminal_write("NEXT CLASS\n");
    terminal_write("Current day unavailable\n\n");

    terminal_write("STUDY PROGRESS\n");
    terminal_write("[");
    if (total_tasks == 0) {
        terminal_write("No tasks yet");
    } else {
        int progress = (completed * 100) / total_tasks;
        int bars = progress / 10;
        int i;
        for (i = 0; i < 10; i++) {
            terminal_write(i < bars ? "#" : "-");
        }
        terminal_write("] ");
        write_int(progress);
        terminal_write("%");
    }
    terminal_write("\n\nTimetable Entries: ");
    write_int(total_schedule);
    terminal_write("\n==========================================\n\n");
}

void productivity_command_overview(void) {
    int pending;
    int completed;
    int total;

    tasks_stats(&pending, &completed, &total);

    terminal_write("\n========================================\n");
    terminal_write("        STUDENT PRODUCTIVITY\n");
    terminal_write("========================================\n\n");

    terminal_write("Tasks\n");
    terminal_write("Pending: ");
    write_int(pending);
    terminal_write("\nCompleted: ");
    write_int(completed);
    terminal_write("\n\nNotes\n");
    terminal_write("Total: ");
    write_int(notes_count());
    terminal_write("\n\nTimetable\n");
    terminal_write("Entries: ");
    write_int(timetable_count());
    terminal_write("\n\nCommands:\n\n");
    terminal_write("task list\nnote list\nschedule list\ndashboard\n");
    terminal_write("========================================\n\n");
}

void productivity_show_dashboard_if_enabled(void) {
    productivity_command_dashboard();
}
