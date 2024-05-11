#include "sched.h"
#include "fs/vfs.h"
#include "lib/kpanic.h"
#include "main.h"
#include "drivers/serial.h"
#include "vmm.h"
#include "pmm.h"
#include "drivers/ps2.h"
#include <ACPI/acpi.h>
#include <elf/elf.h>
#include <fs/tmpfs.h>

struct thread_t *start_of_queue;
struct thread_t *current_thread;
char e[64];
void switch_context(struct StackFrame *frame, struct cpu_context_t *ctx)
{
    *frame = ctx->frame;
}
void save_context(struct StackFrame *frame, struct cpu_context_t *ctx)
{
    ctx->frame = *frame;
}
volatile void switch_task(struct StackFrame *frame)
{
    if (current_thread != NULL)
    {
        
        if (current_thread->next)
        {
            
            if (current_thread->context->frame.cs == 0x40 | (3))
            {
                
                uint64_t kernel_gs_base = 0;
                readmsr(0xC0000101, &kernel_gs_base);
                current_thread->gs_base = kernel_gs_base;
                uint64_t fs = 0;
                readmsr(0xC0000100, &fs);;
                current_thread->fs = fs;
            }

            save_context(frame, current_thread->context);
            current_thread = current_thread->next;
        }
        else
        {
            if (start_of_queue != NULL)
            {
                if (current_thread->context->frame.cs == 0x40 | (3))
                {
                    uint64_t kernel_gs_base = 0;
                    readmsr(0xC0000101, &kernel_gs_base);
                    current_thread->gs_base = kernel_gs_base;
                    uint64_t fs = 0;
                    readmsr(0xC0000100, &fs);;
                    current_thread->fs = fs;
                }
                save_context(frame, current_thread->context);
                
                current_thread = start_of_queue;
            }
            else
            {
                serial_print("NOTHING TO SWITCH TO LOL, RETTING\n");
                return; // QUEUES EMPTY, RETURN AND DO NOTHING
            }
        }
        switch_context(frame, current_thread->context);
        if (current_thread->context->frame.cs == 0x40 | (3))
        {
            writemsr(0xC0000101, (uint64_t)current_thread->gs_base);
            writemsr(0xC0000100, current_thread->fs);
        }
        update_cr3((uint64_t)current_thread->info->pagemap->pml4);
    }
    else
    {
        if (start_of_queue)
        {
            current_thread = start_of_queue;
            
            switch_context(frame, current_thread->context);
            if (current_thread->context->frame.cs == 0x40 | (3))
            {
                writemsr(0xC0000101, (uint64_t)current_thread->gs_base);
                writemsr(0xC0000100, current_thread->fs);
            }
            update_cr3((uint64_t)current_thread->info->pagemap->pml4);
            return;
        }
        serial_print_color("Scheduler: Nothing to switch to!\n", 2);
        return;
    }

}
struct cpu_context_t *new_context(uint64_t entry_func, uint64_t rsp, bool user)

{
    if (!user)
    {
        struct cpu_context_t *new_guy = kmalloc(sizeof(struct cpu_context_t));
        new_guy->frame.rip = entry_func;
        new_guy->frame.rsp = rsp;
        new_guy->frame.rbp = rsp;
        new_guy->frame.rflags = 0x202;
        new_guy->frame.cs = 0x28; // KERNEL CODE
        new_guy->frame.ss = 0x30; // KERNEL DATA
        return new_guy;

    }
    else
    {
        // UNIMPLMENTED LOL
        struct cpu_context_t *new_guy = kmalloc(sizeof(struct cpu_context_t));
        new_guy->frame.rip = entry_func;
        new_guy->frame.rsp = rsp;
        new_guy->frame.rbp = rsp;
        new_guy->frame.rflags = 0x202;
        new_guy->frame.cs = 0x40 | (3); // USER CODE
        new_guy->frame.ss = 0x38 | (3); // USER DATA
        return new_guy;
    }
}
struct thread_t *create_thread(struct cpu_context_t *it)
{
    struct thread_t *thread = kmalloc(sizeof(struct thread_t));
    thread->context = it;
    thread->next = NULL;
    return thread;
}
struct process_info *make_process_info(char *name, int pid)
{
    struct process_info *new_process = kmalloc(sizeof(struct process_info));
    
    strcpy(new_process->name, name);
    new_process->pid = pid;
    new_process->pagemap = &kernel_pagemap; // just set it to the kernels pagemap for now.
    new_process->root_vfs = root->list;
    new_process->cur_working_directory = root->list;

    return new_process;
}
void kthread_start()
{
    while (true)
    {
        if (event_count > 0)
        {
            event_count--;
            struct KeyboardEvent event = events[event_count];
            if (event.printable)
            {
                kprintf("Key Pressed %c\n", event.printable);
            }
            else
            {
                switch (event.key)
                {
                case KEY_ESCAPE:
                    kprintf("KEY ESCAPE PRESSED!\n", event.key);
                    break;
                
                default:
                    break;
                }
            }
            
            
        }
    }
}
struct process_info *get_cur_process_info()
{
    return current_thread->info;
}

void sched_init()
{
    // uint64_t *new_poop = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096);
    struct process_info *e = make_process_info("Keyboard clearer thing", 0);
    uint64_t *new_kstack = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096); // CAUSE STACK GROWS DOWNWARDS
    struct cpu_context_t *ctx = new_context((uint64_t)kthread_start, (uint64_t)new_kstack, false);
    // struct cpu_context_t *timeforpoop = new_context((uint64_t)kthread_poop, (uint64_t)new_poop, pml4, false);
    // serial_print("Created New CPU Context for Kernel Thread\n");
    // serial_print("attempting to create kthread...\n");
    struct thread_t *kthread = create_thread(ctx);
    kthread->info = e;
    
    start_of_queue = kthread;
    uint64_t pag = pmm_alloc_singlep();
    struct pagemap *map = new_pagemap();
    vmm_region_alloc_user(map, 0x1000, NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW | NYA_OS_VMM_USER);
    uint8_t user_program[] = { 0x31, 0xC0, 0x48, 0xC7, 0xC7, 0x0D, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xEB, 0xFE, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x00 };
    memcpy(virt_to_phys((uint64_t)map->pml4 + hhdm_request.response->offset, 0x0) + hhdm_request.response->offset, user_program, sizeof(user_program));
    struct cpu_context_t *ctx2 = new_context(0x0, 0x0 + 4096, true);
    struct thread_t *new_thread = create_thread(ctx2);
    struct process_info *c = make_process_info("User Thread", 1);
    c->pagemap = map;
    new_thread->gs_base = kmalloc(sizeof(struct per_thread_cpu_info_t));
    new_thread->gs_base->kernel_stack_ptr = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096);
    new_thread->gs_base->user_stack_ptr = 0x0 + 4096;
    new_thread->gs_base->ptr = new_thread->gs_base;
    kthread->gs_base = kmalloc(sizeof(struct per_thread_cpu_info_t));
    kthread->gs_base->kernel_stack_ptr = (uint64_t)new_kstack;
    kthread->gs_base->ptr = kthread->gs_base;
    new_thread->info = c;
    kthread->next = new_thread;
    struct vnode *pro = vnode_path_lookup(root->list, "/usr/bin/bash", false, NULL);
    if (!pro)
    {
        kpanic("frick u\n", NULL);
    }
    struct pagemap *for_elf = new_pagemap();
    kprintf("Addr of file data from found vnode: %p\n", ((struct tmpfs_node*)pro->data)->data);
    struct thread_t *fr = load_elf_program(for_elf, 0, pro, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    if (fr)
    {
        fr->next = start_of_queue;
        start_of_queue = fr;
    }
    else {
        kprintf("Failed to load elf program!\n");
    }
    asm ("sti");
    // // lets create another thread trollage
    // struct thread_t *pooper = create_thread(timeforpoop);
    // kthread->next = pooper;
}