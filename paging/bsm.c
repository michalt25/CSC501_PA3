/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
#include <stdio.h>



// Since processes can have multiple address ranges to different backing stores 
// we need to keep track of which backing store a page needs to be read/written to
// when it is being brought into or removed from a frame. 
//
// In order to pull this off we need to keep track of which backing store the
// process was allocated when it was created (vcreate()). Finding the offset
// (the exact page within the store) can be calculated using the virtual page #. 
//
// We will keep track of this mapping (backing store to pid) using a bs_map_t
// structure. A list of these mappings will be maintained for each backing store
// (in the bs_t structure).
//
// The bs_map_t structure holds the following information:
//
//      bsd_t bsid;       // Which BS is mapped here
//      int pid;          // process id using this bs
//      int vpno;         // starting virtual page number
//      int npages;       // number of pages in the store
//      bs_map_t * next;  // A pointer to the next mapping for this bs
//
// With this information we can now accept a pid and a virtual address and find
// which backing store and what offset into the backing store the page is
// located.
//
//   f(pid, virtaddr) => {store, page offset from begin of backing store}



/////*
//// * init_bsm- initialize bsm_tab
//// *
//// */
////SYSCALL init_bsm() {
////    int i;

////    for (i=0; i <= MAX_ID; i++) {
////        bsm_tab[i].bs_status = BSM_UNMAPPED;
////        bsm_tab[i].bs_pid    = 0;
////        bsm_tab[i].bs_vpno   = 0;
////        bsm_tab[i].bs_npages = 0; 
////        bsm_tab[i].bs_sem    = 0;
////    }

////    return OK;
////}

/////*-------------------------------------------------------------------------
//// * get_bsm - get a free entry from bsm_tab 
//// *-------------------------------------------------------------------------
//// */
////int get_bsm() {
////    int i;
////    int entry = -1;

////    for (i=0; i <= MAX_ID; i++) {
////        if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
////            return i;
////        }
////    }

////    return SYSERR;
////}


/////*-------------------------------------------------------------------------
//// * free_bsm - free an entry from bsm_tab 
//// *-------------------------------------------------------------------------
//// */
////SYSCALL free_bsm(int i) {

////    if (i < 0 || i > MAX_ID)
////        return SYSERR;

////    bsm_tab[i].bs_status = BSM_UNMAPPED;

////    return OK;
////}

/////*
//// * bsm_lookup - lookup bsm_tab and find the corresponding entry
//// *
//// *
//// *
//// */
////SYSCALL bsm_lookup(int pid, vir_addr_t vaddr, int* store, int* page) {

////    pd_t * page_dir;
////    pt_t * page_table;

////    int vpno;

////    vpno = VADDR_TO_VPNO(vaddr);

////    // Iterate over backing stores and find the one that contains
////    // the requested page for that process.
////    for (i=0; i <= MAX_ID; i++) {

////        // If backing store not used then continue
////        if (bsm_tab[i].bs_status == BSM_UNMAPPED)
////            continue;

////        // If pid does not own store then continue
////        if (bsm_tab[i].bs_pid != pid)
////            continue;


////        // Make sure the virtual address is within the range of this 
////        // backing store. 
////        if ((vpno >= bsm_tab[i].bs_vpno) &&
////            (vpno < (bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages)))
////        {
////            *store = i;
////            *page  = vpno;
////            return OK;
////        }

////    }

////    return SYSERR;
////}


/////*-------------------------------------------------------------------------
//// * bsm_map - add an mapping into bsm_tab 
//// *-------------------------------------------------------------------------
//// */
////SYSCALL bsm_map(int pid, int vpno, int source, int npages) {

////    int entry;


////  //// XXX what do I do with source? 

////  //// Get a free entry in the bsm table
////  //entry = get_bsm();
////  //if (entry == SYSERR) {
////  //    kprintf("bsm_map - could not get free bsm entry!\n");
////  //    return SYSERR;
////  //}


////    // If this one is already mapped then return error
////    if (bsm_tab[source].bs_status == BSM_MAPPED) {
////        kprintf("Backing store already mapped!\n");
////        return SYSERR;
////    }

////    // Populate all of the date in the bsm entry
////    bsm_tab[source].bs_status = BSM_MAPPED;
////    bsm_tab[source].bs_pid    = pid;
////    bsm_tab[source].bs_vpno   = vpno;
////    bsm_tab[source].bs_npages = npages; 
////    bsm_tab[source].bs_sem    = 0;


////    return OK;
////}


#define OP_FIND    100
#define OP_DELETE  200


#define VPNO_IN_MAP(vp, bsmptr)                      \
                        (((vp) >= (bsmptr)->vpno) && \
                        ((vp) < ((bsmptr)->vpno + (bsmptr)->npages)))


/*
 *  _bs_operate_on_mappings - Does one of two things
 *
 *      1 - Finds a mapping and returns a pointer to the 
 *          mapping (bs_map_t structure). It returns this 
 *          pointer at the memory location of the bsmptr argument.
 *      2 - Find and delete a mapping completely. 
 */
int _bs_operate_on_mapping(int pid, int vpno, int op, bs_map_t * bsmptr) {

    int i;
    bs_t * bsptr;
    bs_map_t * prev;
    bs_map_t * curr;

    //XXX does the vpno have to match exactly or does the vpno
    //simply have to be within range? For now I will assume it just
    //has to be in range

    // Validate op
    if ((op != OP_FIND) && (op != OP_DELETE))
        return SYSERR;

    // Iterate over the lists of maps for each backing store. Each 
    // list corresponds to a different backing store. 'i' will be the 
    // backing store number.
    for (i=0; i < NBS; i++) {

        // Get a pointer to this backing store 'i'
        bsptr = &bs_tab[i];

        // These variables help us iterate over the mapping list.
        prev = NULL;
        curr = bsptr->maps;
    
        // First check the mapping at the head of the maps 
        // to see if it matches.
        if ((curr->pid == pid) && VPNO_IN_MAP(vpno, curr)) {

            // Found it.. act accordingly
            if (op == OP_FIND) {
                bsmptr = curr;
                return OK;
            }

            // OP_DELETE! => Remove mapping
            bsptr->maps = curr->next;
            freemem((struct mblock *)curr, sizeof(bs_map_t));
            return OK;

        }

        // Now iterate over the list and find the one and remove it
        while (curr != NULL) {

            // Is this the entry? 
            if ((curr->pid == pid) && VPNO_IN_MAP(vpno, curr)) {

                // Found it.. act accordingly
                if (op == OP_FIND) {
                    bsmptr = curr;
                    return OK;
                }

                // OP_DELETE! => Remove mapping
                prev->next = curr->next;
                freemem((struct mblock *)curr, sizeof(bs_map_t));
                return OK;

            }

            // Move to next item in the list
            prev = curr;
            curr = curr->next;

        }

    }

    // If we made it here then we did not find a mapping
    return SYSERR;
}



/*
 * bs_add_mapping - Create a new bs_map_t structure and 
 *                  add it to the list of maps for backing
 *                  store bsid.
 */
int bs_add_mapping(bsd_t bsid, int pid, int vpno, int npages) {
    bs_t * bsptr;
    bs_map_t * bsmptr;

    // Get the pointer to the backing store.
    bsptr = &bs_tab[bsid];

    // Get memory for a new bs_map_t 
    bsmptr = (bs_map_t *) getmem(sizeof(bs_map_t));
    if (!bsmptr) {
        kprintf("Error when calling getmem()!\n");
        return SYSERR;
    }

    // XXX do i need to check status of backing store first?
    // bs_tab[bsid].status = 

    // Populate all of the data fields
    bsmptr->bsid   = bsid;
    bsmptr->pid    = pid;
    bsmptr->vpno   = vpno;
    bsmptr->npages = npages;  

    // Finally add the new mapping to the head of that maps list
    bsmptr->next   = bsptr->maps;
    bsptr->maps    = bsmptr;

    return OK;
}




int bs_lookup_mapping(int pid, int vpno, bsd_t * bsid, int * poffset) {
    int rc;
    bs_map_t * bsmptr;


    // Find the mapping using _bs_operate_on_mapping function
    rc = _bs_operate_on_mapping(pid, vpno, OP_FIND, bsmptr);
    if (rc == SYSERR) {
        kprintf("Could not find mapping!\n");
        return SYSERR;
    }

    // bsmptr now points to the mapping. We can get the backing
    // store id from this
    *bsid = bsmptr->bsid;

    // Need to do just a little more work to make sure that vpno
    // correctly matches the vpno of a mapping 
    //
    //
    *poffset = vpno - bsmptr->vpno;

    return OK;
}

////    //int vpno = VADDR_TO_VPNO(vaddr);


////    // Two ways I see to do this at this point
////    //  1 - Go through bs_map_t structures in proc entry
////    //  2 - Go through bs_map_t structures in bs_tab[] entries
////    //
////    //
////    //  Choose 1 for now.
////    //

////    struct pentry * pptr;
////    pptr = &proctab[pid];


////    // Iterate over the lists of backing store mappings for this
////    // process. Each list corresponds to a different backing store. 
////    // 'i' will be the backing store number.
////    for (i=0; i < NBS; i++) {


////        // Get the head of the list of mappings for backing store 'i'
////        item = pptr->map[i];

////        // Iterate through the list searching for an entry that maps
////        // has a mapping for pid and vpno.
////        while (item != NULL) {

////          //// If pid does not own store then continue
////          //// This should not happen considering we are using the
////          //// list in the proc entry
////          //if (item->pid != pid)
////          //    continue;


////            // Is the virtual address is within the range of this 
////            // backing store? If so we found a match!
////            if ((vpno >= item->vpno) &&
////                (vpno < (item->vpno + item->npages)))
////            {
////                *bsid    = i;
////                *poffset = vpno - item->vpno; // Calc offset into backing store
////                return OK;
////            }

////        }

////    }

////    // If we got here then we did not find a mapping
////    return SYSERR;

////}



/*
 * bs_del_mapping - find the mapping for pid,vpno and delete
 *                  it from the mapping list.
 */
int bs_del_mapping(int pid, int vpno) {
    int rc;

    rc = _bs_operate_on_mapping(pid, vpno, OP_DELETE, NULL);
    if (rc == SYSERR) {
        kprintf("Could not delete mapping!\n");
        return SYSERR;
    }

}





////    // Find out which backing store this mapping is a part of
////    rc = bs_lookup_mapping(pid, vpno, &bsid, &page);
////    if (rc == SYSERR)
////        return SYSERR;

////    // Get the pointer to the backing store
////    bsptr = &bs_tab[bsid];



////    // Set up a few variables to iterate over the mapping list
////    prev = NULL;
////    curr = bsptr->maps;
////    
////    // First check the mapping at the head of the backing store 
////    // mapping to see if that is the one we need to remove? 
////    if (curr->pid == pid && curr->vpno == vpno) {
////        bsptr->maps = curr->next;
////        freemem(cur, sizeof(bs_map_t));
////        return OK;
////    } 

////    // Now iterate over the list and find the one and remove it
////    while (curr != NULL) {

////        // Is this the entry? 
////        if (curr->pid == pid && curr->vpno == vpno) {
////            prev->next = curr->next;
////            // free(curr)
////            return OK;
////        }

////        prev = curr;
////        curr = curr->next;

////    }

////}




//////
////// bsm_unmap - delete a mapping from bsm_tab
//////
////SYSCALL bsm_unmap(int pid, int vpno, int flag) {
////    int i;

////    // Iterate over backing stores mappings and find the one that 
////    // maps the pid,vpno tuple.
////    for (i=0; i <= MAX_ID; i++) {

////        // If backing store not used then continue
////        if (bsm_tab[i].bs_status == BSM_UNMAPPED)
////            continue;

////        // If pid does not own store then continue
////        if (bsm_tab[i].bs_pid != pid)
////            continue;

////        // If vpno does not match then continue.
////        if (bsm_tab[i].bs_vpno != vpno)
////            continue;

////        // Ok this is the one we want to unmap
////        bsm_tab[i].bs_status = BSM_UNMAPPED;
////        return OK;

////    }

////    // If we made it here then no bsm was unmapped
////    return SYSERR;

////}


