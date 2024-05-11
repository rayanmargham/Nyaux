#include "tar.h"
#include "main.h"
#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <vmm.h>
#include <pmm.h>
#include <lib/kpanic.h>
#include <drivers/serial.h>
// stolen from osdev wiki :trl:
unsigned int getsize(const char *in)
{
 
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
 
    for (j = 11; j > 0; j--, count *= 8)
        size += ((in[j - 1] - '0') * count);
 
    return size;
 
}
void parse_tar_and_populate_tmpfs(struct limine_file *archive)
{
    struct tar_header *hdr_ptr = archive->address;
    kprintf("first tar header is at address: %p\n", hdr_ptr);
    kprintf("Name: %s\n", hdr_ptr->name);
    kprintf("ramfs: Found Valid initramfs, populating tmpfs...\n");
    struct vnode *roo = kmalloc(sizeof(struct vnode));
    memset(roo, 0, sizeof(struct vnode));
    struct vfs *tmpfs_vfs = kmalloc(sizeof(struct vfs));
    memset(tmpfs_vfs, 0, sizeof(struct vfs));
    root = tmpfs_vfs;
    root->list = roo;
    roo->ops = &tmpfsops;
    struct tmpfs_dir *root_dir = kmalloc(sizeof(struct tmpfs_dir));
    memset(root_dir, 0, sizeof(struct tmpfs_dir));
    roo->type = NYAVNODE_DIR;
    roo->data = root_dir;
    roo->filesystem = &root;
    kprintf("Creating Root VNODE, populating...\n");
    while (hdr_ptr < (archive->address + archive->size) - 1024) // CAUSE OF USTARS WEIRD AH FUCKING THING
    {
        if (strncmp(hdr_ptr->type, "5", strlen(hdr_ptr->type)) == 0)
        {
            struct vnode *notneeded = NULL;
            vfs_create(roo, hdr_ptr->name, 0, &notneeded);
            hdr_ptr = (void*)hdr_ptr + 512 + align_up(getsize(hdr_ptr->filesize_octal), 512);
        }
        else if (strncmp(hdr_ptr->type, "0", strlen(hdr_ptr->type)) == 0)
        {
            struct vnode *notneeded = NULL;
            vfs_create(roo, hdr_ptr->name, 1, &notneeded);
            if (notneeded)
            {
                
                
                notneeded->ops->v_rdwr(notneeded, getsize(hdr_ptr->filesize_octal), 0, (void*)hdr_ptr + 512, 1);
            }
            hdr_ptr = (void*)hdr_ptr + 512 + align_up(getsize(hdr_ptr->filesize_octal), 512);
        }
        else 
        {
            hdr_ptr = (void*)hdr_ptr + 512 + align_up(getsize(hdr_ptr->filesize_octal), 512); // FUCK OFF!
        }
    }
    kprintf("ramfs: initramfs created!\n");
}