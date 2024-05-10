#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fs/vfs.h>
#include "idt.h"


struct cpu_context_t
{
    struct StackFrame frame;
};
struct per_thread_cpu_info_t
{
    uint64_t kernel_stack_ptr; // SINCE SYSCALL DOESNT STORE THE KERNEL STACK, WE MUST USE THIS TO LOAD OUR KERNEL STACK 
    uint64_t user_stack_ptr;
    struct per_thread_cpu_info_t *ptr;
    // WE GET THIS STRUCT BY DOING SWAPGS
    // THAT PUTS IT IN THE KERNELGSBASE MSR FOR US TO READ FROM, WE CAN THEN USE THAT TO FUCKIN LOAD THE KERNEL STACK INTO RSP
};
struct process_info
{
    // PER PROCESS INFO
    struct vnode *root_vfs;
    uint64_t *pagemap; // PAGE MAP OF PROCESS
    struct vnode *cur_working_directory;
    char name[32]; // PROCESS NAME
    uint64_t pid;
};
struct thread_t
{
    char name[32]; // THREAD NAME
    struct cpu_context_t *context;
    struct thread_t *next;
    uint64_t tid; // thread id
    struct process_info *info;
    uint64_t fs; // FOR VARRIABLES PER THREAD, CONFUSING IK BUT MLIBC USES THIS SHIT!!!!
    struct per_thread_cpu_info_t *gs_base;
};
volatile void switch_task(struct StackFrame *frame);
struct process_info *get_cur_process_info();
void sched_init();