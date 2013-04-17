/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

//extern struct pentry proctab[];

/*
 *  vfreemem - free a virtual memory block, returning it to vmemlist
 *
 */
SYSCALL vfreemem(struct mblock* block, unsigned int size) {
    STATWORD ps;    
    struct mblock * curr;
    struct mblock * next;
    struct pentry * pptr;
    unsigned top;

    
    // Make sure they passed us a valid value for size
    if (size==0)
        return SYSERR;

    // Make sure they passed us a valid value for block
    if ((unsigned) block < (unsigned)(NBPG*4096))
        return SYSERR;

    // Get pointer to proctab entry for this proc
    pptr = &proctab[currpid];


    // Round up to a multiple of the size of a memory block
    size = (unsigned)roundmb(size);

    // Disable interrupts
    disable(ps);

    // Iterate through the list until we get to the end or 
    // the next block (p) is greater than the block we are 
    // freeing. Basically this is so we can insert our block
    // back into the list in sorted order.
    curr = &(pptr->vmemlist);
    next = curr->mnext;
    while((next != (struct mblock *) NULL) && (next < block)) { 
         curr=next;
         next=next->mnext;
    }


    // Check for errors
    //     - Is the block already a part of a free block?
    //         - Does curr include part of it?
    //         - Does next include part of it?


    // Check for errors.. See if the block is already a member of the 
    // current block (make sure the current block isn't the head of
    // the list). 
    top = curr->mlen + (unsigned)curr; // The top of the current block
    if ((curr != &(pptr->vmemlist)) && (top > (unsigned)block)) {
        restore(ps);
        return SYSERR;
    }

    // Check for errors.. See if the block is a member of the next
    // block
    if ((next != NULL) && ((size + (unsigned)block) > (unsigned)next)) {
        restore(ps);
        return(SYSERR);
    }

    // Is the top of the current block equal to the start of the
    // block that is being freed? If so then just make the current
    // free block bigger (adjust mlen).
    if ((curr != &(pptr->vmemlist)) && (top == (unsigned)block)) {
            curr->mlen += size;

    // If not then create a new block and add it to the list.
    } else {
        block->mlen = size;
        block->mnext = next;
        curr->mnext = block;
        curr = block;
    }


    // We may now be able to merge this block with the next block
    // if they are now right next to one another. 
    if ((unsigned)(curr->mlen + (unsigned)curr) == (unsigned)next) {
        curr->mlen += next->mlen;
        curr->mnext = next->mnext;
    }

    restore(ps);
    return(OK);
}
