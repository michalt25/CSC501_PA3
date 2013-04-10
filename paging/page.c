/* page.c - manage virtual pages */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


// The first four page tables represent pages of physical memory. This
// memory exists for every process and is thus shared for every
// process. 
pt_t gpt[4] = { 0, 0, 0, 0 };


// At system startup we will create 4 page tables that index
// the first 4096 pages of memory (physical memory). 
int init_page_tables() {
    int i,j;

    // Create the first 4 page tables
    for (i=0; i<4; i++) {

        gpt[i] = pt = pt_alloc();
        if (pt == NULL) {
            kprintf("Could not create page table!\n");
            return SYSERR;
        }

        // fill in data in page table
        //  XXX are these correct defaults?
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

            // What is the base address of this page in memory?
            //
            // We are in the ith page table. That means this page table starts
            // at page i*NENTRIES. We are at the jth page within this
            // page table. Therefore we are at page number (i*NENTRIES + j).
            //
            // Each page contains NBPG bytes, therefore the base address of
            // this page is NBPG*(i*NENTRIES + j) 
            //
            pt[j].p_base  = NBPG*(i*NENTRIES + j); // location of page?
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
    pd_t pd[];

    // Get a new frame of physical memory that will house the page directory.
    frame = alloc_frame(FRM_PD);
    if (frame == NULL) {
      kprintf("Could not get free frame!\n");
      return NULL;
    }

    // fill out rest of frm_tab entry here
    // XXX
    frm_tab[frame].type = FR_DIR;


    // Get the address to the base of the physical memory frame that 
    // we just allocated. That will be the base address of our page
    // directory. 
    pd = (pd_t *) FRMID_TO_ADDR(frame->frmid);


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
        pd[i].pt_base  = gpt[i];  /* location of page table?  */
    }

    return pd;
}

/* 
 * pd_free - Free a page table directory
 */
int pd_free(pd_t * pd) {


    for (i=0; i < NENTRIES; i++)
        pt_free(&pd[i]);

    int frmid = PA2FID(pd);

    frm_free(frmid);

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
    int frame;
    pt_t pt[];

    // Get a new frame of physical memory that will house the page table.
    frame = frm_alloc(FRM_PT);
    if (frame == NULL)
        return NULL;

    // fill out rest of frm_tab entry here
    // XXX
    //frm_tab[frame].type = FR_TBL;


    // Get the address to the base of the physical memory frame that 
    // we just allocated. That address will be the base address of our
    // page table.
    pt = (pt_t *) FID2PA(frame);


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

    int frmid = PA2FID(pt);

    frm_free(frmid);

    return OK;
}

// Find an available entry in the page directory and return
// the index into the page directory.
//
// If none are free return SYSERR
int find_emtpy_pde(pd_t * page_dir) {
    int i;

    // Find a free entry and return the index
    for (i=0; i<NENTRIES; i++)
        if (page_dir[i].pd_avail == 0)
            return i;

    // If we got here then there were no free entries
    return SYSERR;
}
