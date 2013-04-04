/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


// xmmap:
// Much like mmap from unix/linux, xmmap will map a source
// "file" (in our case a "backing store") of size npages pages 
// to the virtual page virtpage.
//
//
// This function does nothing more than create the mapping.
// 
SYSCALL xmmap(int virtpage, bsd_t source, int npages) {
    int rc;

    /* sanity check ! */
    if ((virtpage < 4096) || 
        (source < 0)      || 
        (source > MAX_ID) ||
        (npages < 1)      || 
        (npages >200)
    ) {
        kprintf("xmmap call error: parameter error! \n");
        return SYSERR;
    }


    // Add a mapping to the backing store mapping table
    rc = bsm_map(currpid, virtpage, source, npages);
    if (rc == SYSERR) {
        kprintf("xmmap - could not create mapping!\n");
        return SYSERR;
    }

    return OK;
}



/*
 * xmunmap:
 * removes a virtual memory mapping
 */
SYSCALL xmunmap(int virtpage) {
    int rc;

    if ((virtpage < 4096)) { 
        kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
        return SYSERR;
    }

    // Remove mapping from backing store mapping table
    rc = bsm_unmap(currpid, virtpage, NULL);
    if (rc == SYSERR) {
        kprintf("xmunmap - could not undo mapping!\n");
        return SYSERR;
    }

    return OK;
}

