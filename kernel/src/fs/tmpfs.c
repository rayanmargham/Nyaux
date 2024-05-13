#include "tmpfs.h"
#include "fs/vfs.h"
#include "main.h"
#include <lib/kpanic.h>
#include <sys/types.h>

int vnode_lookup(struct vnode *v, char *part, struct vnode **res)
{
    if (v->type == NYAVNODE_DIR)
    {
        struct tmpfs_dir *dir = v->data;
        struct tmpfs_dir_entry *ok = dir->head;
        if (!ok)
        {
            *res = NULL;
            return -1;
        }
        while (ok)
        {
            if (strcmp(ok->name, part) == 0)
            {
                
                *res = ok->ptr_to_vnode;
                return 0;
            }
            else
            {
                ok = ok->next;
            }
        }
        *res = NULL;
        return -1;
    }
    else
    {
        return -1;
    }
}
int vnode_mkdir(struct vnode *v, const char *name, struct vnode **newdir)
{
    if (v->type == NYAVNODE_DIR)
    {
        struct tmpfs_dir *dir = v->data;
        struct vnode *new = kmalloc(sizeof(struct vnode));
        memset(new, 0, sizeof(struct vnode));
        new->type = NYAVNODE_DIR;
        new->ops = &tmpfsops;
        struct tmpfs_dir *newdirfr = kmalloc(sizeof(struct tmpfs_dir));
        memset(newdirfr, 0, sizeof(struct tmpfs_dir));
        new->data = newdirfr;
        struct tmpfs_dir_entry *new_dir_entry = kmalloc(sizeof(struct tmpfs_dir_entry));
        memset(new_dir_entry, 0, sizeof(struct tmpfs_dir_entry));
        new_dir_entry->name = kmalloc(strlen(name) + 1);
        memset(new_dir_entry->name, 0, strlen(name) + 1);
        strcpy(new_dir_entry->name, name);
        new_dir_entry->ptr_to_vnode = new;
        new_dir_entry->next = dir->head;
        dir->head = new_dir_entry;
        *newdir = new;
        return 0;
    }
    else
    {
        return -1;
    }
}
int vnode_create(struct vnode *dirtocreatefilein, const char *name, struct vnode **newfile)
{
    if (dirtocreatefilein->type == NYAVNODE_DIR)
    {
        // create name lol
        struct tmpfs_dir *dir = dirtocreatefilein->data;
        char *thename = kmalloc(strlen(name) + 1);
        memset(thename, 0, strlen(name) + 1);
        strcpy(thename, name);
        // create dir entry
        struct tmpfs_dir_entry *new_dir_entry = kmalloc(sizeof(struct tmpfs_dir_entry));
        memset(new_dir_entry, 0, sizeof(struct tmpfs_dir_entry));
        new_dir_entry->name = thename;
        new_dir_entry->next = dir->head;
        dir->head = new_dir_entry;

        struct vnode *newnode = kmalloc(sizeof(struct vnode));
        memset(newnode, 0, sizeof(struct vnode));
        newnode->type = NYAVNODE_FILE;
        newnode->data = (struct tmpfs_node*)kmalloc(sizeof(struct tmpfs_node));
        memset(newnode->data, 0, sizeof(struct tmpfs_node));
        newnode->ops = &tmpfsops;
        new_dir_entry->ptr_to_vnode = newnode;
        *newfile = newnode;
        return 0;
    }
    else
    {
        return -1;
    }
}
int vnode_rdwr(struct vnode *v, size_t size_of_buf, size_t offset, void *buf, int rw)
{
    if (v->type == NYAVNODE_FILE)
    {
        if (rw == 0)
        {
            // READ FILE, fail if no file
            struct tmpfs_node *file = v->data;
            if (file->size == 0)
            {
                // file dont exist, failure!
                return 0;
            }
            memcpy(buf, file->data + offset, size_of_buf);
            if (size_of_buf == 0x40)
            {
                
            }
            return 0;
            
        }
        else
        {
            // WRITE FILE, MAKE A FILE IF NO EXIST!!
            struct tmpfs_node *file = v->data;
            if (file->size == 0)
            {
                
                file->data = kmalloc(size_of_buf + offset);
                file->size = size_of_buf + offset;
                
                memset(file->data, 0, file->size);
            }
            else if (size_of_buf + offset > file->size)
            {
                file->data = krealloc(file->data, file->size, size_of_buf + offset);
                file->size = size_of_buf + offset;
            }
            memcpy(file->data + offset, buf, size_of_buf);
            return 0;
        }
    }
    else
    {
        kprintf("clearly not the right vnode\n");
        return -1; // bruh bros trying to writing into a directory :skull:
    }
}
size_t vnode_filesz(struct vnode *v)
{
    struct tmpfs_node *file = v->data;
    if (file)
    {
        return file->size;
    }
    return 0;
}
struct vnodeops tmpfsops = {.v_lookup = vnode_lookup, .v_mkdir = vnode_mkdir, .v_create = vnode_create, .v_rdwr = vnode_rdwr, .v_filesz = vnode_filesz};