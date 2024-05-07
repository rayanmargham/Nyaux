#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fs/vfs.h>
#include "idt.h"

struct cpu_stack_t
{
    uint64_t base;
};
struct cpu_context_t
{
    struct StackFrame frame;
    struct cpu_stack_t stack;
};
struct process_info
{
    // PER PROCESS INFO
    struct vnode *root_vfs;
    uint64_t *pagemap; // PAGE MAP OF PROCESS
    struct vnode *cur_working_directory;
    char *name; // PROCESS NAME
    uint64_t pid;
};
struct thread_t
{
    char *name; // THREAD NAME
    struct cpu_context_t *context;
    struct thread_t *next;
    uint64_t tid; // thread id
    struct process_info *info;
};
volatile void switch_task(struct StackFrame *frame);
struct process_info *get_cur_process_info();
void sched_init();