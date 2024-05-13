#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
// VFS represents a mounted file system
// structure looks like this
// VFS struct ->        VFS Struct
// V                    V
// vnode (directory) -> vnode (directory on different filesystem)
// V
// vnode (file) -> vnode(directory)
//                 V
//                 vnode (file)

struct vfs
{
    struct vfs *next;
    struct vnode *list;
};
struct vnodeops
{
    int (*v_lookup) (struct vnode *v, char *part, struct vnode **result);
    int (*v_mkdir) (struct vnode *v, const char *name, struct vnode **newdir);
    int (*v_rdwr) (struct vnode *v, size_t size_of_buf, size_t offset, void *buf, int rw); // 0 - read; 1 - write
    int (*v_create) (struct vnode *dirtocreatefilein, const char *name, struct vnode **newfile);
    size_t (*v_filesz) (struct vnode *get);
};
enum vnode_type
{
    NYAVNODE_DIR,
    NYAVNODE_FILE,
    NYAVNODE_SYMLINK
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

struct seg_info get_next_part(const char *path);
struct vnode *vnode_path_lookup(struct vnode *cur, const char *path, bool getparent, char *componentlast);
int vfs_create(struct vnode *indir, char *path, int type, struct vnode **res);
int deallocate_fd_from_bitmap(uint8_t *bitmap, int fd);
int allocate_fd_from_bitmap(uint8_t *bitmap, size_t size);