/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

//extern struct pentry proctab[];

/*
 * vgetmem:
 *     allocate virtual heap storage, returning lowest WORD address
 */
WORD *vgetmem(unsigned nbytes) {
    STATWORD ps;    
    struct mblock * curr;
    struct mblock * next;
    struct mblock * leftover;
    struct pentry pptr;


    // Make sure they passed us a valid value
    if (nbytes == 0) 
        return (WORD *)SYSERR;

    // Disable interrupts
    disable(ps);

    // Get pointer to proctab entry for this proc
    pptr = &proctab[currpid];

    // If the free mem list is emtpy then return error
    if (pptr->vmemlist.mnext == NULL) {
        restore(ps);
        return (WORD *)SYSERR;
    }

    // Round up to a multiple of the size of a memory block
    nbytes = (unsigned int) roundmb(nbytes);

    // Iterate through the free list until a block is found
    curr = &(pptr->vmemlist);
    next = pptr->memlist.mnext;

    while(next != (struct mblock *) NULL) { 

        // If the size of the next block is equal to the
        // requested size then remove it from the free list
        // and return a pointer to the base of the memory. 
        if (next->mlen == nbytes) {
            curr->mnext = next->mnext;
            restore(ps);
            return (WORD *)next;
        // else if it has more than enough size then allocate
        // nbytes and add the leftover chunk back to the free list
        } else if ( next->mlen > nbytes ) {
            leftover = (struct mblock *)( (unsigned)next + nbytes );
            curr->mnext = leftover;
            leftover->mnext = next->mnext;
            leftover->mlen = next->mlen - nbytes;
            restore(ps);
            return (WORD *)next;
        }
         curr=next;
         next=next->mnext;
    }

    // If we are here then no mblock was found with enough space
    // for this request. Return error.
    restore(ps);
    kprintf("vgetmem(): Could not allocate vmem\n");
    return (WORD *)SYSERR;
}
