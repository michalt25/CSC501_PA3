/* freemem.c - freemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  freemem  --  free a memory block, returning it to memlist
 *------------------------------------------------------------------------
 */
SYSCALL freemem(struct mblock *block, unsigned size) {
    STATWORD ps;    
    struct  mblock  *p, *q;
    unsigned top;

    if (size==0 || (unsigned)block>(unsigned)maxaddr
        || ((unsigned)block)<((unsigned) &end))
        return(SYSERR);
    size = (unsigned)roundmb(size);
    disable(ps);

    // Iterate through the list until we get to the end or 
    // the next block (p) is greater than the block we are 
    // freeing. Basically this is so we can insert our block
    // back into the list in sorted order.
    for( p=memlist.mnext,q= &memlist;
         p != (struct mblock *) NULL && p < block ;
         q=p,p=p->mnext )
        ;

    
    // Check for errors
    //     - Is the block already a part of a free block?
    //         - Does q include part of it?
    //         - Does p include part of it?
    if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= &memlist) ||
        (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
        restore(ps);
        return(SYSERR);
    }

    // Is the top of the current block equal to the start of the
    // block that is being freed? If so then just make the current
    // free block bigger (adjust mlen).
    if ( q!= &memlist && top == (unsigned)block ) {
            q->mlen += size;

    // If not then create a new block and add it to the list.
    } else {
        block->mlen = size;
        block->mnext = p;
        q->mnext = block;
        q = block;
    }


    // We may now be able to merge this block with the next block
    // if they are now right next to one another. 
    if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
        q->mlen += p->mlen;
        q->mnext = p->mnext;
    }

    restore(ps);
    return(OK);
}
