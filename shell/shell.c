#include "shell.h"
#include "../kernel/terminal.h"

void shell_start(void) {
    terminal_write("studyos> ");
}

