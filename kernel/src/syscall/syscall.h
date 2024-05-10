#pragma once
#include <stdint.h>
#include <stddef.h>


struct syscall_frame
{
    uint64_t r15, r14, r13, r12, rflags, r10, r9, r8;
    uint64_t rdi, rsi, rdx, rip, rbx, rax;
    uint64_t rbp;
};
void RegisterSyscall(int syscall, void (*func)(struct syscall_frame *frame));
void syscall_init();
extern void syscall_handler();