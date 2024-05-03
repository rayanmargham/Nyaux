#include "sched.h"
#include "main.h"
#include "drivers/serial.h"
#include "vmm.h"
#include "pmm.h"

struct thread_t *start_of_queue;
struct thread_t *current_thread;

void switch_context(struct StackFrame *frame, struct cpu_context_t *ctx)
{
    *frame = ctx->frame;
}
volatile void switch_task(struct StackFrame *frame)
{
    if (current_thread != NULL)
    {
        if (current_thread->next)
        {
            current_thread = current_thread->next;
        }
        else
        {
            if (start_of_queue != NULL)
            {
                current_thread = start_of_queue;
            }
            else
            {
                serial_print("NOTHING TO SWITCH TO LOL, RETTING\n");
                return; // QUEUES EMPTY, RETURN AND DO NOTHING
            }
        }
        switch_context(frame, current_thread->context);
        pml4 = current_thread->context->pagemap;
        update_cr3((uint64_t)pml4);
    }
    else
    {
        serial_print_color("Scheduler: Nothing to switch to!\n", 2);
        return;
    }

}
struct cpu_context_t *new_context(uint64_t entry_func, uint64_t rsp, uint64_t pagemap, bool user)

{
    if (!user)
    {
        struct cpu_context_t *new_guy = kmalloc(sizeof(struct cpu_context_t));
        new_guy->frame.rip = entry_func;
        new_guy->frame.rsp = rsp;
        new_guy->frame.rflags = 0x202;
        new_guy->frame.cs = 0x28; // KERNEL CODE
        new_guy->frame.ss = 0x30; // KERNEL DATA
        new_guy->pagemap = pagemap;
        return new_guy;

    }
    else
    {
        // UNIMPLMENTED LOL
        return;
    }
}
struct thread_t *create_thread(struct cpu_context_t *it)
{
    struct thread_t *thread = kmalloc(sizeof(struct thread_t));
    thread->context = it;
    thread->next = NULL;
    return thread;
}
void kthread_start()
{
    serial_print("HELLO WORLD FROM KERNEL THREAD!!!!!\n");
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}
void kthread_poop()
{
    serial_print("HI FROM THE SECOND THREAD, WE HAVE 2 THREADS RUNNING ISNT THAT CRAZY MULTITASKING AM RIGHT???\n");
    asm ("int $32");
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}
void sched_init()
{
    uint64_t *new_poop = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096);
    uint64_t *new_kstack = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096); // CAUSE STACK GROWS DOWNWARDS
    struct cpu_context_t *ctx = new_context((uint64_t)kthread_start, (uint64_t)new_kstack, (uint64_t)pml4, false);
    struct cpu_context_t *timeforpoop = new_context((uint64_t)kthread_poop, (uint64_t)new_poop, pml4, false);
    serial_print("Created New CPU Context for Kernel Thread\n");
    serial_print("attempting to create kthread...\n");
    struct thread_t *kthread = create_thread(ctx);
    start_of_queue = kthread;
    current_thread = start_of_queue;
    // lets create another thread trollage
    struct thread_t *pooper = create_thread(timeforpoop);
    kthread->next = pooper;
}