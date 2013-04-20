/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <paging.h>


/*
 * xmmap:
 * Much like mmap from unix/linux, xmmap will map a source
 * "file" (in our case a "backing store") of size npages pages 
 * to the virtual page virtpage.
 *
 * This function does nothing more than create the mapping.
 */ 
SYSCALL xmmap(int vpno, bsd_t bsid, int npages) {
    int rc;
    bs_t * bsptr;
    STATWORD ps;


#if DUSTYDEBUG
    kprintf("xmmap(%d, %d, %d) for proc %d\n", vpno, bsid, npages, currpid);
#endif

    /* sanity check ! */
    if ((vpno < 4096)        || 
        !IS_VALID_BSID(bsid) || 
        (npages < 1)         || 
        (npages > MAX_BS_PAGES)
    ) {
        kprintf("xmmap call error: parameter error! \n");
        return SYSERR;
    }


    // Disable interrupts
    disable(ps);

    // Make sure the bs has been allocated
    bsptr = &bs_tab[bsid];
    if (bsptr->status == BS_FREE) {
        kprintf("xmmap(): ERROR, BS %d is free!\n", bsptr->bsid);
        kprintf("xmmap - could not create mapping!\n");
        restore(ps);
        return SYSERR;
    }

    // Make sure the bs  is not being used for virtual heap
    if (bsptr->isheap == 1) {
        kprintf("xmmap(): ERROR, BS %d is used as vheap!\n", bsptr->bsid);
        kprintf("xmmap - could not create mapping!\n");
        restore(ps);
        return SYSERR;
    }

    // Add a mapping to the backing store mapping table
    rc = bs_add_mapping(bsid, currpid, vpno, npages);
    if (rc == SYSERR) {
        kprintf("xmmap - could not create mapping!\n");
        restore(ps);
        return SYSERR;
    }

    restore(ps);
    return OK;
}



/*
 * xmunmap:
 * removes a virtual memory mapping
 */
SYSCALL xmunmap(int vpno) {
    int rc;
    struct pentry * pptr;
    bs_map_t * bsmptr;
    STATWORD ps;

#if DUSTYDEBUG
    kprintf("xmunmap(%d) for proc %d\n", vpno, currpid);
#endif

    if ((vpno < 4096)) { 
        kprintf("xmummap call error: vpno (%d) invalid! \n", vpno);
        return SYSERR;
    }

    // Disable interrupts
    disable(ps);

    // Get a pointer to the PCB for current proc
    pptr = &proctab[currpid];

    // Use backing store map to find the store and page offset
    bsmptr = bs_lookup_mapping(currpid, vpno);
    if (bsmptr == NULL) {
        kprintf("xmunmap(): could not find mapping!\n");
        restore(ps);
        return SYSERR;
    }

    // For all frames that are mapped decrease their refcnts
    bsm_frm_cleanup(bsmptr);

    // Remove mapping from maps list
    rc = bs_del_mapping(currpid, vpno);
    if (rc == SYSERR) {
        kprintf("xmunmap(): could not undo mapping!\n");
        restore(ps);
        return SYSERR;
    }

    // Finally must invalidate TLB entries since page table contents 
    // have changed. From intel vol III
    //
    // All of the (nonglobal) TLBs are automatically invalidated any
    // time the CR3 register is loaded.
    set_PDBR(VA2VPNO(pptr->pd));

    restore(ps);
    return OK;
}

