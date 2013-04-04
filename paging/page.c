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
SYSCALL init_page_tables() {
    int i,j;

    // Create the first 4 page tables
    for (i=0; i<4; i++) {

        gpt[i] = page_table = new_page_table();
        if (page_table == SYSERR) {
            kprintf("Could not create page table!\n");
            return SYSERR;
        }

        // fill in data in page table
        //  XXX are these correct defaults
        for (j=0; j<NENTRIES; j++) {
            page_table[j].pt_pres  = 1;        /* page is present?             */ 
            page_table[j].pt_write = 0;        /* page is writable?            */
            page_table[j].pt_user  = 0;        /* is user level protection?    */
            page_table[j].pt_pwt   = 0;        /* write through for this page? */
            page_table[j].pt_pcd   = 0;        /* cache disable for this page? */
            page_table[j].pt_acc   = 0;        /* page was accessed?           */
            page_table[j].pt_dirty = 0;        /* page was written?            */
            page_table[j].pt_mbz   = 0;        /* must be zero                 */
            page_table[j].pt_global= 0;        /* should be zero in 586        */
            page_table[j].pt_avail = 0;        /* for programmer's use         */

            // What is the base address of this page in memory?
            //
            // We are in the ith page table. That means this page table starts
            // at page i*NENTRIES. We are at the jth page within this
            // page table. Therefore we are at page number (i*NENTRIES + j).
            //
            // Each page contains NBPG bytes, therefore the base address of
            // this page is NBPG*(i*NENTRIES + j) 
            page_table[j].pt_base  = NBPG*(i*NENTRIES + j);        /* location of page?        */
        }
    }

    return OK;
}


// new_page_dir - Create a new page table directory
//
// return: 
//          error   - SYSERR
//          success - pd_t* - Base address of directory
//
pd_t * new_page_dir() {
    int i;
    int frame;
    pd_t page_dir[];

    // Get a new frame of physical memory that will house the page directory.
    if ((frame = get_frm()) == SYSERR) {
      kprintf("Could not get free frame!\n");
      return SYSERR;
    }

    // fill out rest of frm_tab entry here
    // XXX
    frm_tab[frame].type = FR_DIR;


    // Get the address to the base of the physical memory frame that 
    // we just allocated. That will be the base address of our page
    // directory. 
    page_dir = (pd_t *) FRM_TO_ADDR(frame);



    // Initialize all entries in the page directory
    for (i=0; i<NENTRIES; i++) {
        page_dir[i].pd_pres  = 0;       /* page table present?      */
        page_dir[i].pd_write = 0;       /* page is writable?        */
        page_dir[i].pd_user  = 0;       /* is user level protection? */
        page_dir[i].pd_pwt   = 0;       /* write through caching for pt?*/
        page_dir[i].pd_pcd   = 0;       /* cache disable for this pt?   */
        page_dir[i].pd_acc   = 0;       /* page table was accessed? */
        page_dir[i].pd_mbz   = 0;       /* must be zero         */
        page_dir[i].pd_fmb   = 0;       /* four MB pages?       */
        page_dir[i].pd_global= 0;       /* global (ignored)     */
        page_dir[i].pd_avail = 0;       /* for programmer's use     */
        page_dir[i].pd_base  = 0;       /* location of page table?  */
    }

    // The first four page directory entries describe the first four
    // page tables (they index memory pages 0-4096 aka the real memory
    // in the system). These page tables were created at system startup and
    // always exist in memory. Their locations are held by the global
    // gpt[] array. 
    for (i=0; i<4; i++) {
        page_dir[i].pd_pres  = 1;       /* page table present?      */
        page_dir[i].pd_avail = 1;       /* for programmer's use     */
        page_dir[i].pd_base  = gpt[i];  /* location of page table?  */
    }

    return page_dir;
}


// new_page_table - Create a new page table 
// return: 
//          error   - SYSERR
//          success - pt_t* - Base address of table
//
pt_t * new_page_table() {
    int i;
    int frame;
    pt_t page_table[];

    // Get a new frame of physical memory that will house the page table.
    frame = get_frm();
    if ((frame = get_frm()) == SYSERR)
        return SYSERR;

    // fill out rest of frm_tab entry here
    // XXX
    frm_tab[frame].type = FR_TBL;


    // Get the address to the base of the physical memory frame that 
    // we just allocated. That will be the base address of our page
    // directory. 
    page_table = (pt_t *) FRM_TO_ADDR(frame);


    // Initialize all entries in the page table
    for (i=0; i<NENTRIES; i++) {
        page_table[i].pt_pres  = 0;        /* page is present?     */
        page_table[i].pt_write = 0;        /* page is writable?        */
        page_table[i].pt_user  = 0;        /* is user level protection? */
        page_table[i].pt_pwt   = 0;        /* write through for this page? */
        page_table[i].pt_pcd   = 0;        /* cache disable for this page? */
        page_table[i].pt_acc   = 0;        /* page was accessed?       */
        page_table[i].pt_dirty = 0;        /* page was written?        */
        page_table[i].pt_mbz   = 0;        /* must be zero         */
        page_table[i].pt_global= 0;        /* should be zero in 586    */
        page_table[i].pt_avail = 0;        /* for programmer's use     */
        page_table[i].pt_base  = 0;        /* location of page?        */
    }

    return page_table;
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
