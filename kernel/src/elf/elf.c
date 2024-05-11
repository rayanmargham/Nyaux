#include "elf.h"
#include "fs/vfs.h"
#include "vmm.h"
#include <main.h>
#include <pmm.h>
#include <lib/kpanic.h>
#include <sched/sched.h>
#include <stdint.h>

#define NEW_THREAD_STACK 0x70000000000
struct thread_t *load_elf_program(struct pagemap *maps, uint64_t base, struct vnode *file, int argc, char **argv, char *envp[], uint64_t entry)
{
    update_cr3((uint64_t)maps->pml4);
    Elf64_ElfHdr hdr = {};

    file->ops->v_rdwr(file, sizeof(hdr), 0, &hdr, 0);
    kprintf("Elf Magic: %c\n", hdr.e_ident[1]);
    kprintf("Elf Header Type: %d\n", hdr.e_type);
    if (hdr.e_type == 2 || hdr.e_type == 3 || hdr.e_ident[7] == 0x00) // EXECUTABLE OR DYNAMIC, MAKE SUER SYSV
    {
        kprintf("IS SYSV ELF WE ROLL :SUNGLASSES\n");
        uint64_t phdr_virt;
        bool interp = false;
        char path[128];
        for (int i = 0; i < hdr.e_phnum; i++)
        {
            Elf64_phdr phdr = {};
            file->ops->v_rdwr(file, sizeof(phdr), hdr.e_phoff + (i * hdr.e_phentsize), &phdr, 0);
            kprintf("Program Header: Type: %d\n", phdr.p_type);
            kprintf("Wants to be loaded at virtual address: %p\n", phdr.p_vaddr);
            switch (phdr.p_type) {
                case 1:
                    kprintf("Difference: %d\n", phdr.p_memsz - phdr.p_filesz);
                    int misaligned = phdr.p_vaddr & 4096;
                    int amount_of_pages = (align_up(phdr.p_memsz + misaligned, 4096)) / 4096;
                    kprintf("Amount of pages to be allocated for this program header: %d\n", amount_of_pages);
                    for (int i = 0; i < amount_of_pages; i++)
                    {
                        void *page = pmm_alloc_singlep();
                        uint64_t virt = align_down(base + phdr.p_vaddr + (i * 0x1000), 4096);
                        map((uint64_t)maps->pml4 + hhdm_request.response->offset, virt, (uint64_t)page, NYA_OS_VMM_USER | NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
                    }
                    
                    file->ops->v_rdwr(file, phdr.p_filesz, phdr.p_offset, (void*)(base + phdr.p_vaddr), 0);
                    break;
                case 6:
                    phdr_virt = base + phdr.p_vaddr;
                    break;
                case 3:
                    interp = true;
                    file->ops->v_rdwr(file, phdr.p_filesz, phdr.p_offset, path, 0);
                    break;
                default:
                    break;
            }
        }
        if (interp == true)
        {
            return load_elf_program(maps, 0x40000000, vnode_path_lookup(root->list, path, false, NULL), argc, argv, envp, base + hdr.e_entry);
        }
        map((uint64_t)maps->pml4 + hhdm_request.response->offset, NEW_THREAD_STACK, (uint64_t)pmm_alloc_singlep(), NYA_OS_VMM_USER | NYA_OS_VMM_PRESENT | NYA_OS_VMM_RW);
        struct cpu_context_t *new_ctx = new_context(hdr.e_entry, NEW_THREAD_STACK + 4096, true);
        struct process_info *e = make_process_info("ELF", 0);
        struct thread_t *new_guy = create_thread(new_ctx);
        new_guy->info = e;
        new_guy->gs_base = kmalloc(sizeof(struct per_thread_cpu_info_t));
        new_guy->gs_base->kernel_stack_ptr = (uint64_t)(((uint64_t)pmm_alloc_singlep() + hhdm_request.response->offset) + 4096);
        new_guy->gs_base->user_stack_ptr = 0x70000000000 + 4096;
        // PUSH AUX VECS
        if (entry)
        {
            new_guy->parameter_window_size += sizeof(Elf64_auxvec);
            Elf64_auxvec entry_aux = {};
            entry_aux.a_type = 9;
        
            entry_aux.a_val = hdr.e_entry;
        
            *(Elf64_auxvec*)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = entry_aux;
        }
        
        new_guy->parameter_window_size += sizeof(Elf64_auxvec);
        Elf64_auxvec phdr_aux = {};
        phdr_aux.a_type = 3;
        phdr_aux.a_val = phdr_virt;
        *(Elf64_auxvec*)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = phdr_aux;
        new_guy->parameter_window_size += sizeof(Elf64_auxvec);
        Elf64_auxvec phent_aux = {};
        phent_aux.a_type = 4;
        phent_aux.a_val = hdr.e_phentsize;
        *(Elf64_auxvec*)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = phent_aux;
        new_guy->parameter_window_size += sizeof(Elf64_auxvec);
        Elf64_auxvec phnum_aux = {};
        phnum_aux.a_type = 5;
        phnum_aux.a_val = hdr.e_phnum;
        *(Elf64_auxvec*)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = phnum_aux;
        new_guy->parameter_window_size += sizeof(Elf64_auxvec);


        // OKAY NOW PUSH FUCKIN ARGC
        new_guy->parameter_window_size += sizeof(argc);
        *(uint32_t*)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = argc;
        new_guy->parameter_window_size += sizeof(argc);
        *(char***)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = argv;
        new_guy->parameter_window_size += sizeof(*argv);
        *(char***)(new_guy->gs_base->user_stack_ptr - new_guy->parameter_window_size) = envp;
        new_guy->parameter_window_size += sizeof(*envp);
        new_guy->gs_base->user_stack_ptr -= new_guy->parameter_window_size;
        kprintf("%p\n", new_guy->gs_base->user_stack_ptr);
        new_guy->gs_base->ptr = new_guy->gs_base;
        e->pagemap = maps;
        return new_guy;

    }
    else 
    {
        update_cr3((uint64_t)kernel_pagemap.pml4);
        return NULL;
    }
    update_cr3((uint64_t)kernel_pagemap.pml4);
    return NULL;
}