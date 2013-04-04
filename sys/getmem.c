/* getmem.c - getmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * getmem  --  allocate heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD *getmem(unsigned nbytes)
{
	STATWORD ps;    
	struct	mblock	*p, *q, *leftover;

	disable(ps);
	if (nbytes==0 || memlist.mnext== (struct mblock *) NULL) {
		restore(ps);
		return( (WORD *)SYSERR);
	}

    // Round up to a multiple of the size of a memory block
	nbytes = (unsigned int) roundmb(nbytes);

    // Iterate through the free list until a block is found
	for (q= &memlist,p=memlist.mnext ;
	     p != (struct mblock *) NULL ;
	     q=p,p=p->mnext
    ) {
        // If the size of the next block is equal to the
        // requested size then remove it from the free list
        // and return a pointer to the base of the memory. 
		if (p->mlen == nbytes) {
			q->mnext = p->mnext;
			restore(ps);
			return( (WORD *)p );
        // else if it has more than enough size then allocate
        // nbytes and add the leftover chunk back to the free list
		} else if ( p->mlen > nbytes ) {
			leftover = (struct mblock *)( (unsigned)p + nbytes );
			q->mnext = leftover;
			leftover->mnext = p->mnext;
			leftover->mlen = p->mlen - nbytes;
			restore(ps);
			return( (WORD *)p );
		}
    }

    // If we are here then no mblock was found with enough space
    // for this request. Return error.
	restore(ps);
	return( (WORD *)SYSERR );
}
