#include "syscall.h"
#include "sched/sched.h"
#include <main.h>
#include <pmm.h>
#include <stdint.h>
#include <vmm.h>

void (*syscall_handlers[332])(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr);

void RegisterSyscall(int syscall, void (*func)(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr))
{
    syscall_handlers[syscall] = func;
}

void syscall_log_mlibc(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    char *msg = frame->rsi;
    if (msg)
    {
        kprintf("userland: %s\n", msg);
    }
}

void syscall_mmap(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    void *hint = frame->rsi;
    size_t size = frame->rdx;
    uint64_t stuff = frame->r10;
    int flags = stuff & 0xFFFFFFFF;
    int proto = (stuff >> 32) & 0xFFFFFFFF;
    size_t fd = frame->r8;
    size_t offset = frame->r9;
    kprintf("Size it wanted to map and allocate: %d, flags: %d\n", size, flags);
    switch (flags)
    {
        case MAP_ANONYMOUS:
            kprintf("Size in pages for what it wanted: %d\n", align_up(size, 4096) / 4096);
            struct pagemap *map = get_current_pagemap_process();
            if (!map)
            {
                frame->rdx = -1;
                return;
            }
            else {
                void *memory = vmm_region_alloc_user(map, size, NYA_OS_VMM_PRESENT | NYA_OS_VMM_USER | NYA_OS_VMM_RW);
                frame->rax = memory;
                frame->rdx = 0;
                return;
            }
            
            break;
        default:
            frame->rdx = -1;
            return;
            break;
    }
    frame->rdx = -1;
}
void syscall_init()
{
    RegisterSyscall(0, syscall_log_mlibc);
    RegisterSyscall(1, syscall_mmap);
}