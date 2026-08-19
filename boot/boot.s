/* StudyOS boot entry (Phase 3 skeleton) */
.section .multiboot
.align 4
.long 0x1BADB002
.long 0x00000000
.long -(0x1BADB002 + 0x00000000)

.section .text
.global _start
.extern kernel_main

_start:
    cli
    mov $stack_top, %esp
    call kernel_main
.hang:
    hlt
    jmp .hang

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:
