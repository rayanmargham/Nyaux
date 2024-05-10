#include "syscall.h"
#include <main.h>
#include <pmm.h>
#include <vmm.h>

void (*syscall_handlers[332])(struct syscall_frame *frame);

void RegisterSyscall(int syscall, void (*func)(struct syscall_frame *frame))
{
    syscall_handlers[syscall] = func;
}

void syscall_log_mlibc(struct syscall_frame *frame)
{
    char *msg = frame->rdi;
    kprintf("mlibc says: %s\n", msg);
}

void syscall_init()
{
    RegisterSyscall(0, syscall_log_mlibc);
}