/* page.c - manage virtual pages */
#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <paging.h>
#include <frame.h>


// The first four page tables represent pages of physical memory. This
// memory exists for every process and is thus shared for every
// process. 
pt_t * gpt[4] = { 0, 0, 0, 0 };


// At system startup we will create 4 page tables that index
// the first 4096 pages of memory (physical memory). 
int init_page_tables() {
    int i,j;
    pt_t * pt;

    // Create the first 4 page tables
    for (i=0; i<4; i++) {

        gpt[i] = pt = pt_alloc();
        if (pt == NULL) {
            kprintf("Could not create page table!\n");
            return SYSERR;
        }

        // fill in data in page table
        for (j=0; j<NENTRIES; j++) {
            pt[j].p_pres  = 1;        /* page is present?             */ 
            pt[j].p_write = 1;        /* page is writable?            */
            pt[j].p_user  = 0;        /* is user level protection?    */
            pt[j].p_pwt   = 0;        /* write through for this page? */
            pt[j].p_pcd   = 0;        /* cache disable for this page? */
            pt[j].p_acc   = 0;        /* page was accessed?           */
            pt[j].p_dirty = 0;        /* page was written?            */
            pt[j].p_mbz   = 0;        /* must be zero                 */
            pt[j].p_global= 0;        /* should be zero in 586        */
            pt[j].p_avail = 0;        /* for programmer's use         */

            // The "base" stores only the upper 20 bits which means
            // that it only really references memory at the granularity 
            // of a page.  What is the # of this page of memory?
            //
            // We are in the ith page table. That means this page table starts
            // at page i*NENTRIES. We are at the jth page within this
            // page table. Therefore we are at page number (i*NENTRIES + j).
            //
            pt[j].p_base  = i*NENTRIES + j; // location of page?

#if DUSTYDEBUG
            if (j == 0) {
                kprintf("Present? %d\t", pt[j].p_pres);
                kprintf("Base is 0x%08x should be 0x%08x\n", pt[j].p_base, (i*NENTRIES + j));
            }
#endif


        }
    }

    return OK;
}


/* 
 * pd_alloc - Create a new page table directory
 *
 * return: 
 *          error   - NULL
 *          success - pd_t* - Base address of directory
 */
pd_t * pd_alloc() {
    int i;
    frame_t * frame;
    pd_t * pd;

    // Get a new frame of physical memory that will house the page directory.
    frame = frm_alloc();
    if (frame == NULL) {
      kprintf("Could not get free frame!\n");
      return NULL;
    }

    // fill out rest of frm_tab entry here
    // XXX
    frame->type = FRM_PD;


    // Get the address to the base of the physical memory frame that 
    // we just allocated. That will be the base address of our page
    // directory. 
    pd = (pd_t *) FID2PA(frame->frmid);


    // Initialize all entries in the page directory
    for (i=0; i<NENTRIES; i++) {
        pd[i].pt_pres  = 0;       /* page table present?      */
        pd[i].pt_write = 0;       /* page is writable?        */
        pd[i].pt_user  = 0;       /* is user level protection? */
        pd[i].pt_pwt   = 0;       /* write through caching for pt?*/
        pd[i].pt_pcd   = 0;       /* cache disable for this pt?   */
        pd[i].pt_acc   = 0;       /* page table was accessed? */
        pd[i].pt_mbz   = 0;       /* must be zero         */
        pd[i].pt_fmb   = 0;       /* four MB pages?       */
        pd[i].pt_global= 0;       /* global (ignored)     */
        pd[i].pt_avail = 0;       /* for programmer's use     */
        pd[i].pt_base  = 0;       /* location of page table?  */
    }

    // The first four page directory entries describe the first four
    // page tables (they index memory pages 0-4096 aka the real memory
    // in the system). These page tables were created at system startup and
    // always exist in memory. Their locations are held by the global
    // gpt[] array. 
    for (i=0; i<4; i++) {
        pd[i].pt_pres  = 1;       /* page table present?      */
        pd[i].pt_write = 1;       /* page is writable?        */
        pd[i].pt_avail = 1;       /* for programmer's use     */
        pd[i].pt_base  = VA2VPNO((unsigned int)gpt[i]);  /* location of page table?  */


#if DUSTYDEBUG
        kprintf("Page table %d is at location \tpage:%d\taddr:0x%08x..\t", 
                i, VA2VPNO((unsigned int) gpt[i]), (unsigned int) gpt[i]);
        kprintf("1st page at page: %d\n", ((pt_t *)VPNO2VA(pd[i].pt_base))[0].p_base);
#endif

    }

    return pd;
}

/* 
 * pd_free - Free a page table directory
 */
int pd_free(pd_t * pd) {
    int i;

    for (i=0; i < NENTRIES; i++)
        pt_free((pt_t *)&pd[i]);

    //int frmid = PA2FID((int)pd);

    frm_free(PA2FP(pd));

    return OK;
}


/*
 * pt_alloc - Create a new page table 
 * return: 
 *          error   - NULL
 *          success - pt_t * - Base address of table
 */
pt_t * pt_alloc() {
    int i;
    frame_t * frame;
    pt_t * pt;

    // Get a new frame of physical memory that will house the page table.
    frame = frm_alloc();
    if (frame == NULL)
        return NULL;

    // fill out rest of frm_tab entry here
    frame->type = FRM_PT;


    // Get the address to the base of the physical memory frame that 
    // we just allocated. That address will be the base address of our
    // page table.
    pt = (pt_t *) FID2PA(frame->frmid);


    // Initialize all entries in the page table
    for (i=0; i<NENTRIES; i++) {
        pt[i].p_pres  = 0;        /* page is present?     */
        pt[i].p_write = 0;        /* page is writable?        */
        pt[i].p_user  = 0;        /* is user level protection? */
        pt[i].p_pwt   = 0;        /* write through for this page? */
        pt[i].p_pcd   = 0;        /* cache disable for this page? */
        pt[i].p_acc   = 0;        /* page was accessed?       */
        pt[i].p_dirty = 0;        /* page was written?        */
        pt[i].p_mbz   = 0;        /* must be zero         */
        pt[i].p_global= 0;        /* should be zero in 586    */
        pt[i].p_avail = 0;        /* for programmer's use     */
        pt[i].p_base  = 0;        /* location of page?        */
    }

    return pt;
}

/* 
 * pt_free - Free a page table
 */
int pt_free(pt_t * pt) {

    frm_free(PA2FP(pt));

    return OK;
}

/* 
 * p_free - Free a page table entry
 */
int p_free(pt_t * pt) {

    pt->p_pres  = 0;        /* page is present?     */
    pt->p_write = 0;        /* page is writable?        */
    pt->p_user  = 0;        /* is user level protection? */
    pt->p_pwt   = 0;        /* write through for this page? */
    pt->p_pcd   = 0;        /* cache disable for this page? */
    pt->p_acc   = 0;        /* page was accessed?       */
    pt->p_dirty = 0;        /* page was written?        */
    pt->p_mbz   = 0;        /* must be zero         */
    pt->p_global= 0;        /* should be zero in 586    */
    pt->p_avail = 0;        /* for programmer's use     */
    pt->p_base  = 0;        /* location of page?        */

    return OK;
}


/*
 * p_invalidate - Invalidate any page table entries that
 *                map to physical frame with base at addr
 */
int p_invalidate(int addr) {
    int proc, i, j;
    struct pentry * pptr;
    pd_t * pd;
    pt_t * pt;
    int page;
    frame_t * ptframe;
    int dirty = 0; // Keeps up with whether the page is dirty

    page = VA2VPNO(addr);

    for (proc=0; proc<NPROC; proc++) {

        // If this proc doesn't exist skip
        pptr = &proctab[proc];
        if (pptr->pstate == PRFREE)
            continue;

        // Get the page dir for this process
        // and iterate over entries
        //
        // Note: Must start at 4 because we don't 
        // want to invalidate entries from first 4 
        // page tables.
        pd = pptr->pd; 
        for (i=4; i<NENTRIES; i++) {

            // Is this page table present?
            if (pd[i].pt_pres) {
                pt = VPNO2VA(pd[i].pt_base);

                // Iterate over page table entries
                for (j=0; j<NENTRIES; j++) {

                    if (pt[j].p_pres &&
                        (pt[j].p_base == page)
                    ) {

                                if (pt[j].p_dirty)
                                    dirty = 1;
#if DUSTYDEBUG
                                kprintf("Invalidating pt entry for base frame" 
                                        " %d dirty:%d - proc:%d pt:%d offset:%d\n", 
                                        PA2FID(addr), dirty, proc, i, j);
#endif
                                p_free(&pt[j]);

                                // update refcnt of pt
                                ptframe = PA2FP(pt);
                                ptframe->refcnt--;
                    }

                }

            }

        }

    }

    return dirty;

}
