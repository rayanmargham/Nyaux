#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "idt.h"

struct cpu_stack_t
{
    uint64_t base, ptr;
};
struct cpu_context_t
{
    uint64_t *pagemap;
    struct StackFrame frame;
    struct cpu_stack_t stack;
};
struct thread_t
{
    struct cpu_context_t *context;
    struct thread_t *next;
    uint64_t tid; // thread id
    uint64_t pid; // IF MULTIPLE THREADS BELONG TO ONE PROCESS, THEIR PID WILL BE THE SAME
};
volatile void switch_task(struct StackFrame *frame);
void sched_init();