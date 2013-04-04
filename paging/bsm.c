/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>



// Since processes can have multiple address ranges to different backing stores 
// we need to keep track of which backing store a page needs to be read/written to
// when it is being brought into or removed from a frame. 
//
// In order to pull this off we need to keep track of which backing store the
// process was allocated when it was created (vcreate()). Finding the offset
// (the exact page within the store) can be calculated using the virtual page #. 
//
// We will keep track of this mapping (backing store to pid) using a bs_map_t
// structure. The bsm_tab array holds these mappings for the 8 backing stores. A
// bs_map_t structure holds the following information:
//
//      int bs_status;            /* MAPPED or UNMAPPED       */
//      int bs_pid;               /* process id using this slot   */
//      int bs_vpno;              /* starting virtual page number */
//      int bs_npages;            /* number of pages in the store */
//      int bs_sem;               /* semaphore mechanism ?    */
//
// With this information we can now accept a pid and a virtual address and find
// which backing store and what offset into the backing store the page is
// located.
//
//   f(pid, virtaddr) => {store, page offset from begin of backing store}





// Table with entries representing backing stores
bs_map_t bsm_tab[MAX_ID + 1];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm() {
    int i;

    for (i=0; i <= MAX_ID; i++) {
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid    = 0;
        bsm_tab[i].bs_vpno   = 0;
        bsm_tab[i].bs_npages = 0; 
        bsm_tab[i].bs_sem    = 0;
    }

    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
int get_bsm() {
    int i;
    int entry = -1;

    for (i=0; i <= MAX_ID; i++) {
        if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
            return i;
        }
    }

    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i) {

    if (i < 0 || i > MAX_ID)
        return SYSERR;

    bsm_tab[i].bs_status = BSM_UNMAPPED;

    return OK;
}

/*
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *
 *
 *
 */
SYSCALL bsm_lookup(int pid, vir_addr_t vaddr, int* store, int* page) {

    pd_t * page_dir;
    pt_t * page_table;

    int vpno;

    vpno = VADDR_TO_VPNO(vaddr);

    // Iterate over backing stores and find the one that contains
    // the requested page for that process.
    for (i=0; i <= MAX_ID; i++) {

        // If backing store not used then continue
        if (bsm_tab[i].bs_status == BSM_UNMAPPED)
            continue;

        // If pid does not own store then continue
        if (bsm_tab[i].bs_pid != pid)
            continue;


        // Make sure the virtual address is within the range of this 
        // backing store. 
        if ((vpno >= bsm_tab[i].bs_vpno) &&
            (vpno < (bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages)))
        {
            *store = i;
            *page  = vpno;
            return OK;
        }

    }

    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages) {

    int entry;


  //// XXX what do I do with source? 

  //// Get a free entry in the bsm table
  //entry = get_bsm();
  //if (entry == SYSERR) {
  //    kprintf("bsm_map - could not get free bsm entry!\n");
  //    return SYSERR;
  //}


    // If this one is already mapped then return error
    if (bsm_tab[source].bs_status == BSM_MAPPED) {
        kprintf("Backing store already mapped!\n");
        return SYSERR;
    }

    // Populate all of the date in the bsm entry
    bsm_tab[source].bs_status = BSM_MAPPED;
    bsm_tab[source].bs_pid    = pid;
    bsm_tab[source].bs_vpno   = vpno;
    bsm_tab[source].bs_npages = npages; 
    bsm_tab[source].bs_sem    = 0;


    return OK;
}



//
// bsm_unmap - delete a mapping from bsm_tab
//
SYSCALL bsm_unmap(int pid, int vpno, int flag) {
    int i;

    // Iterate over backing stores mappings and find the one that 
    // maps the pid,vpno tuple.
    for (i=0; i <= MAX_ID; i++) {

        // If backing store not used then continue
        if (bsm_tab[i].bs_status == BSM_UNMAPPED)
            continue;

        // If pid does not own store then continue
        if (bsm_tab[i].bs_pid != pid)
            continue;

        // If vpno does not match then continue.
        if (bsm_tab[i].bs_vpno != vpno)
            continue;

        // Ok this is the one we want to unmap
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        return OK;

    }

    // If we made it here then no bsm was unmapped
    return SYSERR;

}


