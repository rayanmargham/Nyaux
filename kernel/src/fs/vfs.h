#pragma once
#include <stdint.h>
#include <stddef.h>
// VFS represents a mounted file system
// structure looks like this
// VFS struct ->        VFS Struct
// V                    V
// vnode (directory) -> vnode (directory on different filesystem)
// V
// vnode (file) -> vnode(directory)
//                 V
//                 vnode (file)
struct seg_info
{
    const char *seg;
    size_t len;
};
struct vfs
{
    struct vfs *next;
    struct vnode *list;
};
struct vnodeops
{
    int (*v_lookup) (struct vnode *v, struct seg_info *seg, struct vnode **result);
    int (*v_mkdir) (struct vnode *v, const char *name, struct vnode **newdir);
    int (*v_rdwr) (struct vnode *v, size_t size_of_buf, size_t offset, void *buf, int rw); // 0 - read; 1 - write
    int (*v_create) (struct vnode *dirtocreatefilein, const char *name, struct vnode **newfile);
};
enum vnode_type
{
    NYAVNODE_DIR,
    NYAVNODE_FILE
};
struct vnode
{
    int refcount; // HOW MANY DIRECTORY ENTRIES REFERENCE THSI VNODE
    struct vfs *filesystem;
    struct vnodeops *ops;
    void *data; 
    enum vnode_type type;
};
extern struct vfs *root;
void test_vfs();