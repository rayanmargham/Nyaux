#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vmm.h>
#include <fs/vfs.h>
typedef uint64_t	Elf64_Addr;
typedef uint16_t	Elf64_Half;
typedef uint16_t	Elf64_SHalf;
typedef uint64_t	Elf64_Off;
typedef uint32_t	Elf64_Word;
typedef uint64_t	Elf64_Xword;

typedef struct {
    unsigned char e_ident[16];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry; // Entry Point
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_ElfHdr;

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_phdr;

typedef struct
{
    int a_type;
    long a_val;
} Elf64_auxvec;
struct thread_t *load_elf_program(struct pagemap *maps, uint64_t base, struct vnode *file, int argc, char **argv, char *envp[], uint64_t entry);