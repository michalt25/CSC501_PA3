/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
#include <stdio.h>
#include <bs.h>
#include <frame.h>

#define OP_FIND    100
#define OP_DELETE  200


#define VPNO_IN_MAP(vp, bsmptr)                      \
                        (((vp) >= (bsmptr)->vpno) && \
                        ((vp) < ((bsmptr)->vpno + (bsmptr)->npages)))


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



/*
 * bs_cleanproc - Clean up any mappings for the given process
 *
 *  -  When a process dies the following should happen.
 *     1. All frames which currently hold any of its pages should be written to
 *        the backing store and be freed.
 *     2. All of it's mappings should be removed from the backing store map.
 */
int bs_cleanproc(int pid) {
    int i;
    bs_t * bsptr;
    bs_map_t * prev;
    bs_map_t * curr;

#if DUSTYDEBUG
    kprintf("bs_cleanproc %d\n", pid);
#endif

    // Iterate over the lists of maps for each backing store. Each 
    // list corresponds to a different backing store. 'i' will be the 
    // backing store number.
    for (i=0; i < NBS; i++) {

        // Get a pointer to this backing store 'i'
        bsptr = &bs_tab[i];

        // If this backing store is not being used then move on
        if (bsptr->status == BS_FREE)
            continue;

        // These variables help us iterate over the mapping list.
        prev = NULL;
        curr = bsptr->maps;

        // Now iterate over the list and find the one and remove it
        while (curr) {

            // Does this mapping belong to proc pid?
            if (curr->pid == pid) {

                // Decrease refcnts for any frames
                bsm_frm_cleanup(curr);

                // If this is the head of the list act accordingly
                if (prev == NULL) {
                    bsptr->maps = curr->next;
                    freemem((struct mblock *)curr, sizeof(bs_map_t));
                    curr = bsptr->maps;
                } else {
                    prev->next = curr->next;
                    freemem((struct mblock *)curr, sizeof(bs_map_t));
                    curr = prev->next;
                }

            } else {

                // Move to next item in the list
                prev = curr;
                curr = curr->next;

            }

        }


        // Check this backing store to see if it now how no mappings.
        // If so then we can free the backing store.
        if (bsptr->maps == NULL)
            bs_free(bsptr);

    }

    // If we made it here then we did not find a mapping
    return OK;
}


/*
 * bsm_frm_cleanup 
 *      - Given a mapping decrefcnt all frames that are 
 *        in this mapping
 */
int bsm_frm_cleanup(bs_map_t * bsmptr) {
    bs_t * bsptr;
    frame_t * prev;
    frame_t * curr;

#if DUSTYDEBUG
    kprintf("bsm_frm_cleanup for bsid:%d pid:%d vpno:%d npages:%d\n",
            bsmptr->bsid, bsmptr->pid, bsmptr->vpno, bsmptr->npages);
#endif

    // Get a pointer to the bs_t structure for the backing store
    bsptr = &bs_tab[bsmptr->bsid];

    // Go through the frames that this bs has and release the frame
    // that corresponds to this virtual frame.
    prev = NULL;
    curr = bsptr->frames;
    while (curr) {

        // Does this frame match?
        if (curr->bspage < bsmptr->npages)
            frm_decrefcnt(curr);

        // Move to next frame in list
        prev = curr;
        curr = curr->bs_next;
    }

}

/*
 *  _bs_operate_on_mappings - Does one of two things
 *
 *      1 - Finds a mapping and returns a pointer to the 
 *          mapping (bs_map_t structure). It returns this 
 *          pointer at the memory location of the bsmptr argument.
 *      2 - Find and delete a mapping completely. 
 */
int _bs_operate_on_mapping(int pid, int vpno, int op, bs_map_t ** bsmptrptr) {

    int i;
    bs_t * bsptr;
    bs_map_t * prev;
    bs_map_t * curr;

    // Validate op
    if ((op != OP_FIND) && (op != OP_DELETE))
        return SYSERR;

    // Iterate over the lists of maps for each backing store. Each 
    // list corresponds to a different backing store. 'i' will be the 
    // backing store number.
    for (i=0; i < NBS; i++) {

        // Get a pointer to this backing store 'i'
        bsptr = &bs_tab[i];

        // If this backing store is not being used or if this store
        // has no maps then move on
        if (bsptr->status == BS_FREE || bsptr->maps == NULL)
            continue;

        // These variables help us iterate over the mapping list.
        prev = NULL;
        curr = bsptr->maps;
    
        // First check the mapping at the head of the maps 
        // to see if it matches.
        if ((curr->pid == pid) && VPNO_IN_MAP(vpno, curr)) {

            // Found it.. act accordingly
            if (op == OP_FIND) {
                *bsmptrptr = curr;
                return OK;
            }

            // OP_DELETE! => Remove mapping
            bsptr->maps = curr->next;
            freemem((struct mblock *)curr, sizeof(bs_map_t));
            return OK;

        }

        // Now iterate over the list and find the one and remove it
        while (curr) {

            // Is this the entry? 
            if ((curr->pid == pid) && VPNO_IN_MAP(vpno, curr)) {

                // Found it.. act accordingly
                if (op == OP_FIND) {
                    *bsmptrptr = curr;
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

#if DUSTYDEBUG
    kprintf("bs_add_mapping(%d, %d, %d, %d) for proc %d\n", bsid, pid, vpno, npages, pid);
#endif

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




bs_map_t * bs_lookup_mapping(int pid, int vpno) {
    int rc;
    bs_map_t * bsmptr;

#if DUSTYDEBUG
    kprintf("bs_lookup_mapping(%d, %d) for proc %d..\t", pid, vpno, pid);
#endif

    // Find the mapping using _bs_operate_on_mapping function
    rc = _bs_operate_on_mapping(pid, vpno, OP_FIND, &bsmptr);
    if (rc == SYSERR) {
        kprintf("\nCould not find mapping!\n");
        return NULL;
    }



#if DUSTYDEBUG
    kprintf("found bsid:%d pid:%d vpno:%d npages:%d\n",
    bsmptr->bsid,
    bsmptr->pid,
    bsmptr->vpno,
    bsmptr->npages);
#endif

    return bsmptr;
}


/*
 * bs_del_mapping - find the mapping for pid,vpno and delete
 *                  it from the mapping list.
 */
int bs_del_mapping(int pid, int vpno) {
    int rc;

#if DUSTYDEBUG
    kprintf("bs_del_mapping(%d, %d) for proc %d\n", pid, vpno, pid);
#endif

    rc = _bs_operate_on_mapping(pid, vpno, OP_DELETE, NULL);
    if (rc == SYSERR) {
        kprintf("Could not delete mapping!\n");
        return SYSERR;
    }

    return OK;
}
