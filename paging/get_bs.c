#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*
 * This call requests from the page server a new backing store 
 * with id bs_id of size npages (in pages, not bytes). If the page 
 * server is able to create the new backingstore, or a backingstore 
 * with this ID already exists, the size of the new or existing 
 * backingstore is returned. This size is in pages. If a size of 0 
 * is requested, or the pageserver encounters an error, SYSERR is 
 * returned. 
 *
 * Also, since our backing stores are 1M in size and our page size is
 * 4K, the npages will be no more than 256.
 */
int get_bs(bsd_t bsid, unsigned int npages) {
    bs_t * bsptr;
    STATWORD ps;

#if DUSTYDEBUG
    kprintf("get_bs(%d, %d) for proc %d\n", bsid, npages, currpid);
#endif

    // Make sure the bsid is appropriate
    if (!IS_VALID_BSID(bsid))
        return SYSERR;

    // If a size of 0 is requested, return SYSERR
    if (npages == 0)
        return SYSERR;

    // If a size greater than 256 is requested, return SYSERR
    if (npages > MAX_BS_PAGES)
        return SYSERR;

    // Get the pointer to the requested bs
    bsptr = &bs_tab[bsid];

    // If store is part of heap then return SYSERR
    if ((bsptr->status == BS_USED) && (bsptr->isheap))
        return SYSERR;

    // If store bsid already exists return the size of existing 
    // backing store.
    if (bsptr->status == BS_USED)
        return bsptr->npages;

    // backing store does not exist.. create it.
    disable(ps);
    if (bsptr->status == BS_FREE) {
        bs_alloc(bsid, npages); // Not checking ret code
        restore(ps);
        return npages;
    }

    // If we got here then something is wrong.
    restore(ps);
    return SYSERR;
}
