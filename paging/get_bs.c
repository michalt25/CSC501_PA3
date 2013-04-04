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
int get_bs(bsd_t bs_id, unsigned int npages) {


    // If a size of 0 is requested, return SYSERR
    if (npages == 0)
        return SYSERR;

    // If a size greater than 256 is requested, return SYSERR
    if (npages > 256)
        return SYSERR;


    // If store bs_id already exists return the size of existing 
    // backing store.
    if (bsm_tab[bs_id].bs_status == BSM_MAPPED)
        return bsm_tab[bs_id].bs_npages;


    // backing store does not exist.. create it.



    // Get an empty entry in the page directory for this process
    struct  pentry  *pptr;
    pptr = &proctab[currpid];
    pd_t * page_dir;
    page_dir = (pd_t *) pptr->pdbr;
    int entry;
    entry = find_empty_pde(page_dir);
    if (entry == SYSERR) {
        return SYSERR;
    }
    
    // Build up page table etc.. 
    pt_t * page_table;
    page_table = new_page_table();
    if (page_table == SYSERR) {
        kprintf("Could not create page table!\n");
        return SYSERR;
    }


    base = BSID_TO_ADDR(bs_id);

    // fill in data in page table
    //  XXX are these correct defaults
    for (i=0; i<NENTRIES; i++) {
        page_table[i].pt_pres  = 0;        /* page is present?             */ 
        page_table[i].pt_write = 0;        /* page is writable?            */
        page_table[i].pt_user  = 0;        /* is user level protection?    */
        page_table[i].pt_pwt   = 0;        /* write through for this page? */
        page_table[i].pt_pcd   = 0;        /* cache disable for this page? */
        page_table[i].pt_acc   = 0;        /* page was accessed?           */
        page_table[i].pt_dirty = 0;        /* page was written?            */
        page_table[i].pt_mbz   = 0;        /* must be zero                 */
        page_table[i].pt_global= 0;        /* should be zero in 586        */
        page_table[i].pt_avail = 1;        /* for programmer's use         */


        if (i > npages)
            page_table[i].pt_avail = 0;

        // What is the base address of this page in memory?
        page_table[i].pt_base = base + i*NBPG;
    }


    // Put the base addr of the page table into the
    // entry in the page directory.
    page_dir[entry].pd_base = page_table;



    return npages;
}


