// bs.c - manage the backing store structures

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>



// Table with entries representing backing stores
bs_t bs_tab[NBS];

/*
 * init_bstab - initialize bs_tab
 */
int init_bstab() {
    int i;

    for (i=0; i < NBS; i++) {

        bs_tab[i].bsid   = i; // Never needs to be set again
        bs_tab[i].status = BS_FREE;
        bs_tab[i].isheap = 0;
        bs_tab[i].npages = 0;
        bs_tab[i].maps   = NULL;
        bs_tab[i].frames = NULL;

    }

    return OK;
}

////SYSCALL init_bs(
////                bs_t * bsptr, 
////                  int status, 
////                  int isheap, 
////             bs_map_t * maps,
////            frame_t * frames,
////) {

////    bsptr.status = status;
////    bsptr.isheap = isheap;
////    bsptr.npages = npages;
////    bsptr.maps   = maps;
////    bsptr.frames = frames;
////    return OK;
////}

/*
 * bs_alloc - Allocate a new backing store identified by
 *            bsid. If bs bsid is not free return SYSERR
 */
int bs_alloc(bsd_t bsid, int npages) {
    int i;
    bs_t * bsptr;

    // Get pointer to backing store
    bsptr = &bs_tab[bsid];

    // Is it already being used?
    if (bsptr->status == BS_USED)
        return SYSERR;

    // Initialize the data
    bsptr->status = BS_USED;
    bsptr->isheap = 0; // XXX 
    bsptr->npages = npages;
    bsptr->owners = NULL; // XXX 
    bsptr->frames = NULL; // XXX

    return bsptr;
}


/*
 * get_free_bs - Get a free backing store that can 
 *               accommodate npages
 */
bs_t * get_free_bs(int npages) {
    int i;

    // Make sure they gave us a valid # of pages
    if (npages >= MAX_BS_PAGES)
        return NULL

    // Find the first free backing store with at least
    // npages free.
    for (i=0; i < NBS; i++)
        if (bs_tab[i].status == BS_FREE)
            if (bs_tab[i].npages >= npages)
                break;

    // If i==NBS then we did not find one
    if (i == NBS) {
        kprintf("Could not find free backing store\n");
        return NULL;
    }

    // Return a pointer to the backing store entry
    return &bs_tab[i];

}

/*
 * bs_free - Free the backing store identified by bsid
 */
int bs_free(bsd_t bsid) {
    int i;
    bs_t     * bsptr;
    bs_map_t * prev;
    bs_map_t * curr;

    // Get pointer to the backing store
    bsptr = &bs_tab[bsid];

    // Make sure to free any of the mappings that
    // the backing store had.
    prev = NULL;
    curr = bsptr->maps;
    while (curr) {
        prev = curr;
        curr = curr->next;
        freemem(prev, sizeof(bs_map_t));
    }

    bsptr->status = BS_FREE;
    bsptr->isheap = 0;
    bsptr->npages = 0;
    bsptr->frames = NULL;
    bsptr->maps   = NULL;


    return OK;
}
