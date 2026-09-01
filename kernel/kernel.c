#include "kernel.h"
#include "memory.h"
#include "terminal.h"
#include "../drivers/keyboard.h"
#include "../filesystem/filesystem.h"
#include "../security/security.h"
#include "../shell/shell.h"

void kernel_early_init(void) {
    terminal_clear();
    memory_init();
    keyboard_init();
    filesystem_init();
    security_init();
}

void kernel_main(void) {
    kernel_early_init();
    shell_start();
}
