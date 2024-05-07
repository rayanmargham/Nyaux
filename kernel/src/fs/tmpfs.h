#pragma once
#include <stdint.h>
#include <stddef.h>
#include "vfs.h"
#include <main.h>
#include <drivers/serial.h>
#include <pmm.h>
#include <vmm.h>

struct tmpfs_dir_entry 
{
    char *name;
    struct vnode *ptr_to_vnode; // COULD BE FILE OR ANOTHER DIRECTORY
    struct tmpfs_dir_entry *next;
};
struct tmpfs_dir
{
    struct tmpfs_dir_entry *head; // LIST OF DIR ENTRYS!
};
struct tmpfs_node
{
    void *data;
    size_t size;
};
extern struct vnodeops tmpfsops;