#include "vfs.h"
#include <main.h>
#include <drivers/serial.h>
#include <pmm.h>
#include <vmm.h>
#include "tmpfs.h"

struct vfs *root;


char *strchr(const char *p, int ch)
{
	char c;

	c = ch;
	for (;; ++p) {
		if (*p == c)
			return ((char *)p);
		if (*p == '\0')
			return (NULL);
	}
	/* NOTREACHED */
}
struct seg_info get_next_part(const char *path)
{
    char *ptr = path;
    char *test = strchr(ptr, '/');
    if (test != NULL)
    {
        if (test[1] != NULL)
        {
            test = test + 1;
        }
        char *next = strchr(test, '/');
        if (next != NULL)
        {
            size_t length = next - test;
            struct seg_info poop = {test, length};
            return poop;
            
        }
        else
        {
            size_t length = strlen(test);
            struct seg_info poop = {test, length};
            return poop;
        }
        
    }
    else
    {
        struct seg_info fake = {NULL, NULL};
        return fake;
    }
}
struct vnode *vnode_path_lookup(const char *path)
{
    struct vnode *cur_vnode = NULL;
    char *ptr = path;
    if (path[0] == '/')
    {
        cur_vnode = root->list;
    }
    else
    {
        // SHIT
        return NULL; // NOT IMPLMENTED 
        
    }
    if (path[strlen(path) - 1] == '/' || path[strlen(path) - 1] == '\\')
    {
        ptr[strlen(path) - 1] = '\0';
    }
    while (cur_vnode)
    {
        struct seg_info next_seg = get_next_part(ptr);
        if (next_seg.seg == NULL)
        {
            break;
        }
        ptr = next_seg.seg; // e
        struct vnode *res;
        
        cur_vnode->ops->v_lookup(cur_vnode, &next_seg, &res);

        
        if (res)
        {
            if (res->type == NYAVNODE_DIR)
            {
                cur_vnode = res; // keep going down the directory
            }
            else
            {
                return res; // this is a file 1000000% looool
            }
        }
        else
        {
            return NULL; // NOT FOUND? IG?
        }
        
    }
    return cur_vnode; // PROBS NULL BUT COULD ALSO BE A DIRECTORY.

}
void test_vfs()
{
    struct vfs ro = {};
    root = &ro;
    struct tmpfs_dir actual_root = {.head = NULL};
    struct vnode root_vnode = {
        .ops = &tmpfsops,
        .type = NYAVNODE_DIR,
        .data = &actual_root
    };
    root->list = &root_vnode;
    struct vnode *homedir = NULL;
    root_vnode.ops->v_mkdir(&root_vnode, "home", &homedir);
    struct vnode *test = vnode_path_lookup("/home");
    if (test)
    {
        kprintf("FOUND directory /home!\n");
    }
    struct vnode *meowtxt = NULL;
    test->ops->v_create(test, "meow.txt", &meowtxt);
    struct vnode *findmeow = vnode_path_lookup("/home/meow.txt");
    if (findmeow)
    {
        kprintf("Found File /home/meow.txt!\n");
    }
    char m[] = "YES I AM A NERD";
    findmeow->ops->v_rdwr(findmeow, sizeof(m), 0, m, 1);
    char buff[sizeof(m)];
    findmeow->ops->v_rdwr(findmeow, sizeof(buff), 0, buff, 0);
    kprintf("Contents of meow.txt: %s\n", buff);
}