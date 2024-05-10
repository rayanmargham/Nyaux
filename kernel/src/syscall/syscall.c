#include "syscall.h"
#include <main.h>
#include <pmm.h>
#include <vmm.h>

void (*syscall_handlers[332])(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr);

void RegisterSyscall(int syscall, void (*func)(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr))
{
    syscall_handlers[syscall] = func;
}

void syscall_log_mlibc(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    kprintf("addr of msg: %p\n", frame->rdi);
    char *msg = frame->rdi;
    kprintf("mlibc says: %s\n", msg);
    kprintf("Kernel Stack for this Thread %p\n", ptr->kernel_stack_ptr);
}

void syscall_init()
{
    RegisterSyscall(0, syscall_log_mlibc);
}