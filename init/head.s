.section .entry
.global _start

_start:
/* reset data segment */
    movw $(2<<3), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    lea  init_task + 4096, %eax
    movl %eax, %esp
    movl %eax, %ebp

    jmp $(1<<3), $main


