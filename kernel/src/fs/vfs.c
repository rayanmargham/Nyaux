#include "vfs.h"
#include <main.h>
#include <drivers/serial.h>
#include <pmm.h>
#include <vmm.h>
#include "tmpfs.h"
#include <stdbool.h>

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

struct vnode *vnode_path_lookup(struct vnode *cur, const char *path, bool getparent, char *componentlast)
{
    
    struct vnode *cur_vnode = cur;
    size_t pathlen = strlen(path);
    char *work_with_me = kmalloc(pathlen + 1);
    memset(work_with_me, 0, pathlen + 1);
    strcpy(work_with_me, path);
    for (int i = 0; i < pathlen; ++i)
    {
        if (work_with_me[i] == '/')
        {
            work_with_me[i] = '\0';
        }
    }
    for (int i = 0; i < pathlen; ++i)
    {
        if (work_with_me[i] == '\0')
        {
            continue;
        }
        struct vnode *res = NULL;
        char *component = &work_with_me[i];
        size_t len_of_comp = strlen(component);
        bool islast = i + len_of_comp == pathlen;
        if (!islast)
        {
            // stolen from astral os :trl: thanks mathew
            // this just deals with cases of paths given like /shit/ or smthing idk
            // i dont understand it but THROW IT IN ANYWAY!
            // i hate dealing with path parsing :angry:
            int j;
            for (j = i + len_of_comp; i < pathlen && work_with_me[j] == '\0'; ++j)
            islast = j == pathlen;
        }
        if (islast && getparent == true)
        {
            
            strcpy(componentlast, component);
            return cur_vnode;
        }
        cur_vnode->ops->v_lookup(cur_vnode, component, &res);


        if (res)
        {
            if (res->type == NYAVNODE_DIR)
            {
                cur_vnode = res; // keep going down the directory
                i += len_of_comp;
            }
            else
            {
                return res; // this is a file 1000000% looool
            }
        }
        else
        {
            strcpy(componentlast, component);
            if (cur_vnode)
            {
                return cur_vnode;
            }
            else {
                return NULL;
            }
        }
        
    }
    return cur_vnode; // PROBS NULL BUT COULD ALSO BE A DIRECTORY.

}

int vfs_create(struct vnode *indir, char *path, int type, struct vnode **res)
{
    if (path[0] == '\0')
    {
        return -1;
    }
    char *component = kmalloc(strlen(path) + 1);
    struct vnode *our_par = vnode_path_lookup(indir, path, true, component);
    if (!our_par)
    {
        kfree(component, strlen(path) + 1);
        return -1;
    }
    struct vnode *created = NULL;
    if (type == 0)
    {
        // CREATE DIRECTORY
        our_par->ops->v_mkdir(our_par, component, &created);
        kfree(component, strlen(path) + 1);
        *res = created;
        return 0;
    }
    else if (type == 1)
    {
        our_par->ops->v_create(our_par, component, &created);
        kfree(component, strlen(path + 1));
        *res = created;
        return 0;
    }
    else
    {
        
        // not supported
        return -1;
    }

}
void test_vfs()
{
    
}