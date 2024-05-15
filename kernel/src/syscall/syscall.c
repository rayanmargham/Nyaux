#include "syscall.h"
#include "ACPI/acpi.h"
#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "sched/sched.h"
#include <main.h>
#include <pmm.h>
#include <stdint.h>
#include <vmm.h>
#include <lib/kpanic.h>

void (*syscall_handlers[332])(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr);

void RegisterSyscall(int syscall, void (*func)(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr))
{
    syscall_handlers[syscall] = func;
}

void syscall_log_mlibc(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    char *msg = (char*)frame->rsi;
    kprintf("userland: %s\n", msg);
    if (strcmp(msg, "MLIBC PANIC\n") == 0)
    {
        kpanic("Mlibc Panicked!\n", NULL);
    }
}
void syscall_write(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    int fd = frame->rsi;
    void *buf = (void*)frame->rdx;
    size_t count = frame->r10;
    if (fd == 1)
    {
        // SINCE WE DONT HAVE DEVICES WE JUST WRITE DIRECTLY TO FLANTERM
        flanterm_write(ctx, buf, count);
        frame->rdx = 0;
        frame->rax = count;
        return;
    }
    else {
        kprintf("syscall_write: failure at fd: %d\n", fd);
        frame->rdx = -1;
        return;
    }
    
}
void syscall_mmap(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    kprintf("syscall_mmap()\n");
    void *hint = (void*)frame->rsi;
    size_t size = frame->rdx;
    uint64_t stuff = frame->r10;
    int flags = stuff & 0xFFFFFFFF;
    int proto = (stuff >> 32) & 0xFFFFFFFF;
    size_t fd = frame->r8;
    size_t offset = frame->r9;
    if (flags == MAP_ANONYMOUS)
    {
        struct pagemap *map = get_current_pagemap_process();
            if (!map)
            {
                frame->rdx = -1;
                return;
            }
            else {
                void *memory = vmm_region_alloc_user(map, size, NYA_OS_VMM_PRESENT | NYA_OS_VMM_USER | NYA_OS_VMM_RW);
                frame->rax = (uint64_t)memory;
                frame->rdx = 0;
                kprintf("syscall_mmap: gave address from base %p\n", memory);
                return;
            }
    }
    else if (flags == (MAP_ANONYMOUS | MAP_FIXED) || flags == (MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE))
    {
        struct pagemap *map = get_current_pagemap_process();
            if (!map)
            {
                frame->rdx = -1;
                return;
            }
            else {
                if (proto & PROT_EXEC)
                {
                    
                    void *memory = mmap_range(map, (uint64_t)hint, size, NYA_OS_VMM_PRESENT | NYA_OS_VMM_USER | NYA_OS_VMM_USER | NYA_OS_VMM_RW);
                    frame->rax = (uint64_t)memory;
                    frame->rdx = 0;
                    kprintf("syscall_mmap: gave address from base %p\n", memory);
                }
                else {
                    void *memory = mmap_range(map, (uint64_t)hint, size, NYA_OS_VMM_PRESENT | NYA_OS_VMM_USER | NYA_OS_VMM_USER | NYA_OS_VMM_RW | NYA_OS_VMM_XD);
                    frame->rax = (uint64_t)memory;
                    frame->rdx = 0;
                    kprintf("syscall_mmap: gave address from base %p\n", memory);
                }
                
                return;
            }
    }
    frame->rdx = -1;
}
void syscall_openat(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    int dirfd = frame->rsi;
    char *pathname = (char*)frame->rdx;
    kprintf("syscall_openat(): looking for path %s\n", pathname);
    switch (dirfd)
    {
        case -100:
            struct process_info *process = get_cur_process_info();
            struct vnode *ptr = vnode_path_lookup(process->cur_working_directory, pathname, false, NULL);
            
            if (ptr)
            {
                
                int fd = allocate_fd_from_bitmap(process->descriptor_bitmap, 256);
                kprintf("syscall_openat: allocated fd %d\n");
                process->Descriptors[fd].ptr = ptr;
                process->Descriptors[fd].offset = 0;
                frame->rax = fd;
                frame->rdx = 0;
                return;
            }
            else {
                frame->rdx = -1;
                kprintf("Couldn't find this path\n");
                return;
            }
            break;
        default:
            kpanic("STUB!\n", NULL);
            frame->rdx = -1;
            return;
            break;
    }
}
void syscall_read(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    kprintf("syscall_read()\n");
    int fd = frame->rsi;
    void *buf = (void*)frame->rdx;
    size_t size_of_buf = frame->r10;
    struct process_info *pro = get_cur_process_info();
    struct FileDescriptor *d = &pro->Descriptors[fd];
    if (d)
    {
        if (d->ptr)
        {
            kprintf("Buf: %p Offset: %d, fd: %d\n", buf, d->offset, fd);
            kprintf("Addr of vnode: %p\n", d->ptr);
            int hm = d->ptr->ops->v_rdwr(d->ptr, size_of_buf, d->offset, buf, 0);
            if (hm != -1)
            {
                kprintf("old offset is %d ", d->offset);
                d->offset += hm;
                kprintf("new offset is %d\n", d->offset);
                frame->rdx = 0;
                return;
            }
            frame->rdx = -1;
            return;
        }
        else {
            kprintf("Clearly exists\n");
            frame->rdx = -1;
            return;
        }
    }
    frame->rdx = -1;
    return;
}
void syscall_close(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    kprintf("syscall_close()\n");
    int fd = frame->rsi;
    struct process_info *pro = get_cur_process_info();
    struct FileDescriptor *d = &pro->Descriptors[fd];
    deallocate_fd_from_bitmap(pro->descriptor_bitmap, fd);
    d->ptr = NULL;
    d->offset = 0;
    frame->rdx = 0;
    return;
}
void syscall_seek(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    kprintf("syscall_seek()\n");
    int fd = frame->rsi;
    int offset = frame->rdx;
    int flag = frame->r10;
    struct process_info *pro = get_cur_process_info();
    struct FileDescriptor *d = &pro->Descriptors[fd];
    switch (flag)
    {
        case 0:
            if (d)
            {
                d->offset = offset;
                frame->rdx = 0;
                frame->rax = d->offset;
                return;
            }
            else 
            {
                frame->rdx = -1;
                return;
            }
            break;
        case 1:
            if (d)
            {
                d->offset = d->offset + offset;
                frame->rdx = 0;
                frame->rax = d->offset;
                return;
            }
            else 
            {
                frame->rdx = -1;
                return;
            }
            break;
        case 2:
            if (d)
            {
                d->offset = (int)d->ptr->ops->v_filesz(d->ptr) + offset;
                frame->rdx = 0;
                frame->rax = d->offset;
                return;
            }
            else 
            {
                frame->rdx = -1;
                return;
            }
            break;
        default:
            kprintf("%d\n", flag);
            kprintf("Flags are wrong?\n");
            frame->rdx = -1;
            return;
            break;
    }
}
void syscall_fs(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    kprintf("syscall_fs()\n");
    void *q = (void*)frame->rsi;
    kprintf("syscall_fs(): setting fs base to %p\n", q);
    writemsr(0xC0000100, (uint64_t)q);
}
void syscall_dup(struct syscall_frame *frame, struct per_thread_cpu_info_t *ptr)
{
    int fd = frame->rsi;
    kprintf("he requested to duplicate file descriptor %d\n", fd);
    frame->rdx = -1; // error out
    return;
}
void syscall_init()
{
    RegisterSyscall(0, syscall_log_mlibc);
    RegisterSyscall(1, syscall_mmap);
    RegisterSyscall(2, syscall_openat);
    RegisterSyscall(3, syscall_read);
    RegisterSyscall(4, syscall_close);
    RegisterSyscall(5, syscall_seek);
    RegisterSyscall(6, syscall_fs);
    RegisterSyscall(7, syscall_write);
    RegisterSyscall(8, syscall_dup);
}