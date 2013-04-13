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


    kprintf("xmmap(%d, %d, %d) for proc %d\n", vpno, bsid, npages, currpid);

    /* sanity check ! */
    if ((vpno < 4096)   || 
        (bsid < 0)      || 
        (bsid > MAX_ID) ||
        (npages < 1)    || 
        (npages >200)
    ) {
        kprintf("xmmap call error: parameter error! \n");
        return SYSERR;
    }


    // Add a mapping to the backing store mapping table
    rc = bs_add_mapping(bsid, currpid, vpno, npages);
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
SYSCALL xmunmap(int vpno) {
    int rc;
    int bsoffset;
    bsd_t bsid;
    bs_t * bsptr;
    frame_t * prev;
    frame_t * curr;
    struct pentry * pptr;

    kprintf("xmunmap(%d) for proc %d\n", vpno, currpid);

    // Get a pointer to the PCB for current proc
    pptr = &proctab[currpid];

    if ((vpno < 4096)) { 
        kprintf("xmummap call error: vpno (%d) invalid! \n", vpno);
        return SYSERR;
    }

    // Use backing store map to find the store and page offset
    rc = bs_lookup_mapping(currpid, vpno, &bsid, &bsoffset);
    if (rc == SYSERR) {
        kprintf("xmunmap(): could not find mapping!\n");
        return SYSERR;
    }


// From TA

////for (i = 0; i < map->npages; i++) {
////    /*There is no mapping of this virtual page*/
////    if (walk_pt(FP2PA(proc->pd), map->vpno + i, &walk) == SYSERR) {
////        continue;
////    }

////    /*This virtual page is mapped to some free frame, 
////     release the frame.*/
////    bs_put_frame(id, i);
////    free_pte(&walk);
////}











    // Get a pointer to the bs_t structure for the backing store
    bsptr = &bs_tab[bsid];

    // Go through the frames that this bs has and release the frame
    // that corresponds to this virtual frame.
    prev = NULL;
    curr = bsptr->frames;
    while (curr) {

        // Does this frame match?
        if (curr->bspage == bsoffset) {

            // Remove the frame from the list. Act differently
            // depending on if the frame is the head of the list or
            // not
            if (prev == NULL)
                bsptr->frames = curr->next;
            else
                prev->next = curr->next;

            // Free the frame
            frm_free(curr->frmid);
        } 

        // Move to next frame in list
        prev = curr;
        curr = curr->next;
    }

    // Remove mapping from maps list
    rc = bs_del_mapping(currpid, vpno);
    if (rc == SYSERR) {
        kprintf("xmunmap(): could not undo mapping!\n");
        return SYSERR;
    }

    // Finally must invalidate TLB entries since page table contents 
    // have changed. From intel vol III
    //
    // All of the (nonglobal) TLBs are automatically invalidated any
    // time the CR3 register is loaded.
    set_PDBR(VA2VPNO(pptr->pd));

    return OK;
}

