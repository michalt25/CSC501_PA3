/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>



// When writing out a dirty page the only way to figure out which backing store
// (virtual page and process) a dirty frame belongs to would be to traverse the
// page tables of every process looking for a frame location that corresponds to
// the frame we wish to write out. This is inefficient. To prevent this we use
// an inverted page table (maps frames to pages) named frm_tab  that contains 
// fr_map_t structures which contain the following data:
//
//      int fr_status;            /* MAPPED or UNMAPPED       */
//      int fr_pid;               /* process id using this frame  */
//      int fr_vpno;              /* corresponding virtual page no*/
//      int fr_refcnt;            /* reference count      */
//      int fr_type;              /* FR_DIR, FR_TBL, FR_PAGE  */
//      int fr_dirty;
//      void *cookie;             /* private data structure   */
//      unsigned long int fr_loadtime;    /* when the page is loaded  */
//
// With this information we can easily find the pid and virtual page # of the
// page held within any frame. From that we can easily find the backing store 
// (using the backing store maps within bsm_tab[]) and compute which page within
// the backing store corresponds with the page in the frame. 
//
//
// Note: This table also holds some information useful in page replacement decisions


// Table with entries reprensenting frame
frame_t frm_tab[NFRAMES];

// Used for FIFO frame replacement policy
frame_t * frm_fifo_head;

frame_t * _frm_evict();
frame_t * _frm_evict_fifo();
frame_t * _frm_evict_aging();


/*
 * frm_decrefcnt - decrease the reference count of a frame
 *                 and free if it possible 
 */
int frm_decrefcnt(frame_t * frame) {

#if DUSTYDEBUG
    kprintf("frm_decrefcnt(): frm %d type %d - cnt %d -> %d\n", 
            frame->frmid, frame->type, frame->refcnt, frame->refcnt - 1);
#endif 

    frame->refcnt--;

    if (frame->refcnt == 0)
        frm_free(frame);

    return OK;
}

/*
 * frm_cleanlists - Go through the lists and clean up frames that are no
 *                  longer in use. Only need to provide head of
 *                  bs_next list.. frm_fifo_head is global. If NULL is
 *                  passed in for bs_next_head then we won't go
 *                  through that list.
 */
int frm_cleanlists(void * bspointer) {
    bs_t * bsptr;
    frame_t * prev;
    frame_t * curr;

    bsptr = (bs_t *) bspointer;

    // Go through fifo_list!
    prev = NULL;
    curr = frm_fifo_head;
    while (curr) {
        if (curr->status == FRM_FREE) {

#if DUSTYDEBUG
            kprintf("frm_cleanlists(): removing frm %d from fifo_next list\n", 
                    curr->frmid);
#endif 

            // Remove the frame from the list. Act differently
            // depending on if the frame is the head of the list or
            // not
            if (prev == NULL) {
                frm_fifo_head = curr->fifo_next;
                curr = frm_fifo_head;
            } else {
                prev->fifo_next = curr->fifo_next;
                curr = prev->fifo_next;
            }

            continue;
        } 

        // Move to next frame in list
        prev = curr;
        curr = curr->fifo_next;
    }


    // Go through bs_next list!
    // If they passed us NULL then just return
    if (bsptr == NULL)
        return OK;

    prev = NULL;
    curr = bsptr->frames;
    while (curr) {
        if (curr->status == FRM_FREE) {

#if DUSTYDEBUG
            kprintf("frm_cleanlists(): removing frm %d from bs_next list\n", 
                    curr->frmid);
#endif 

            // Remove the frame from the list. Act differently
            // depending on if the frame is the head of the list or
            // not
            if (prev == NULL) {
                bsptr->frames = curr->bs_next;
                curr = bsptr->frames;
            } else {
                prev->bs_next = curr->bs_next;
                curr = prev->bs_next;
            }

            continue;
        } 

        // Move to next frame in list
        prev = curr;
        curr = curr->bs_next;
    }


    return OK;
}

/*
 * frm_free - free a frame 
 */
int frm_free(frame_t * frame) {

    bs_t * bsptr;
    pt_t * pte;
    int dirty;

    if (!IS_VALID_FRMID(frame->frmid))
        return SYSERR;

#if DUSTYDEBUG
    kprintf("frm_free(): Freeing frame %d\n", frame->frmid);
#endif 

    // Invalidate any page table entries for this frame
    dirty = p_invalidate(FID2PA(frame->frmid));

    // If this frame is mapped from a backing store
    // then write the data back to the backing store
    if (frame->type == FRM_BS && dirty) {

        // Make sure the bsid is valid
        if (!IS_VALID_BSID(frame->bsid))
            return SYSERR;
        
        write_bs(FID2PA(frame->frmid), frame->bsid, frame->bspage);

    }

    // Set the frame status as free and clean it up from any
    // lists it may be in.
    frame->status = FRM_FREE;

    // Clean this frame up from any lists it may be in
    if (frame->type == FRM_BS)
        frm_cleanlists(&bs_tab[frame->bsid]);
    else
        frm_cleanlists(NULL);


    // Any other cleanup that needs to be done..
    frame->type   = FRM_FREE;
    frame->refcnt = 0;
    frame->age    = 0;
    frame->bsid   = -1;
    frame->bspage = 0;
    frame->bs_next   = NULL;
    frame->fifo_next = NULL;

    return OK;
}

/*
 * _frm_evict - Find a frame to evict from memory
 */
frame_t * _frm_evict() {
    int i;
    frame_t * frame;


    //  2. Else, Pick a page to replace.
    //  3. Using the inverted page table, get vp, the virtual page number of the page to be replaced.
    //  4. Let a be vp*4096 (the first virtual address on page vp).
    //  5. Let p be the high 10 bits of a. Let q be bits [21:12] of a.
    //  6. Let pid be the pid of the process owning vp.
    //  7. Let pd point to the page directory of process pid.
    //  8. Let pt point to the pid's pth page table.
    //  9. Mark the appropriate entry of pt as not present.
    //  10. If the page being removed belongs to the current process, 
    //      invalidate the TLB entry for the page vp using the invlpg 
    //      instruction (see Intel Volume III/II).
    //  11. In the inverted page table decrement the reference count of the 
    //      frame occupied by pt. If the reference count has reached zero, you 
    //      should mark the appropriate entry in pd as being not present.
    //      This conserves frames by keeping only page tables which are necessary.
    //  12. If the dirty bit for page vp was set in its page table you must 
    //      do the following:
    //          - Using the backing store map find the store and page
    //            offset within store given pid and a. If the lookup fails,
    //            something is wrong. Print an error message and kill the 
    //            process pid.
    //          - Write the page back to the backing store.
    
    // Iterate over the frames
    for (i=0; i < NFRAMES; i++) {

        frame = &frm_tab[i];

        // Is this frame free, if so use it
        if (frame->status == FRM_FREE)
            return frame;

    }

    if (grpolicy() == FIFO) {
        if ((frame = _frm_evict_fifo()) == NULL)
            return NULL;
    } else {
        if ((frame = _frm_evict_aging()) == NULL)
            return NULL;
    }

    if (debugTA)
        kprintf("_frm_evict(): Evicting frame %d\n", frame->frmid);


    // Free the frame
    frm_free(frame);

    return frame;
}

/*
 * _frm_evict_fifo - Evict using FIFO replacement policy
 */
frame_t * _frm_evict_fifo() {
    frame_t * prev;
    frame_t * curr;
    int found = 0;

#if DUSTYDEBUG
        kprintf("_frm_evict_fifo(): Evicting frame\n");
#endif 

    // we must force one page out of memory to free up space.
    //
    // Release the frame closest to the head of the fifo that 
    // isn't a page directory or page table
    prev = NULL;
    curr = frm_fifo_head;
    while (curr) {
        if (curr->type == FRM_BS) {
            found = 1;
            break;
        }
        prev = curr;
        curr = curr->fifo_next;
    }

    // If we didn't find anything then return null
    if (!found)
        return NULL;

    // return the pointer to the free frame
    return curr;
}

/*
 * _frm_evict_aging - Evict using AGING replacement policy
 *
 * Note: The way the algorithm works out the smallest value
 *       for age actually means it is the oldest so we will 
 *       find a frame with the smallest value for age to evict.
 */
frame_t * _frm_evict_aging() {
    frame_t * prev;
    frame_t * curr;
    frame_t * candidate;
    frame_t framestruct;

#if DUSTYDEBUG
        kprintf("_frm_evict_aging(): Evicting frame\n");
#endif 

    // Initially our candidate will be set to the fake frame
    // with age 255;
    candidate = &framestruct;
    candidate->age = 255;

    // Find the frame with largest age.
    prev = NULL;
    curr = frm_fifo_head;
    while (curr) {
        if (curr->type == FRM_BS) {
            if (curr->age < candidate->age)
                candidate = curr;
        }
        prev = curr;
        curr = curr->fifo_next;
    }

    // Did we find a real candidate? If not.. error
    if (candidate == &framestruct)
        return NULL;

    return candidate;
}

/*
 * init_frm_tab - initialize frm_tab
 *
 */
int init_frmtab() {
    int i;

    // To start out the FIFO frame replacement policy 
    // head pointer will be null;
    frm_fifo_head = NULL;

    // Initialize all of the information in the frame table
    for (i=0; i < NFRAMES; i++) {
        frm_tab[i].frmid  = i;        // frame id/index
        frm_tab[i].status = FRM_FREE; // Current status
        frm_tab[i].type   = FRM_FREE; // Type
       // frm_tab[i].pid  = 0;      /* process id using this frame  */
        frm_tab[i].refcnt = 0;        // reference count
        frm_tab[i].age    = 0;        // when page is loaded (in ticks)
        frm_tab[i].bspage = 0;
        frm_tab[i].bsid   = -1;
        frm_tab[i].pte    = NULL;
        frm_tab[i].fifo_next = NULL;
        frm_tab[i].bs_next   = NULL;
    }

    return OK;
}

/*
 * frm_alloc - get a free frame according page replacement policy.
 * 
 */
frame_t * frm_alloc() {
    int i;
    frame_t * frame;
    frame_t * curr;


    frame = _frm_evict();
    if (frame == NULL) {
        kprintf("frm_alloc(): failed to find/evict frame\n"); 
        return NULL;
    }

#if DUSTYDEBUG
    kprintf("Allocating frame \tid:%d \taddr:0x%08x\n", 
            frame->frmid,
            FID2PA(frame->frmid));
#endif

    // Populate data in the frame_t table
    frame->status = FRM_USED; // Current status
    frame->refcnt = 1;        // reference count
    frame->age    = 0;
    frame->bsid   = -1;
    frame->bspage = 0;
    frame->fifo_next = NULL;
    frame->bs_next = NULL;

    // Add frame to end of fifo. 
    if (frm_fifo_head == NULL) {
        frm_fifo_head = frame;
    } else {
        curr = frm_fifo_head;
        while (curr->fifo_next != NULL)
            curr = curr->fifo_next;
        curr->fifo_next = frame;
    }

    return frame;

}


/*
 * frm_find_bspage - Iterate over the frame table and determine 
 *                   if the backing store page (determined by bsid,
 *                   bsoffset) is already in physical memory. If so
 *                   return a pointer to the frame_t struct for that
 *                   frame
 */
frame_t * frm_find_bspage(int bsid, int bsoffset) {
    int i;
    frame_t * frame;

    // Iterate over the frames
    for (i=0; i < NFRAMES; i++) {

        frame = &frm_tab[i];

        // Is this frame free, if so skip
        if (frame->status == FRM_FREE)
            continue;

        // Is this a backing store page. If not skip
        if (frame->type != FRM_BS)
            continue;

        // Does the bsid/bsoffset match? If so.. bingo
        if (frame->bsid == bsid && frame->bspage == bsoffset) {
#if DUSTYDEBUG
            kprintf("Frame %d for bs:%d bspage:%d already mapped\n", 
                frame->frmid,
                bsid,
                bsoffset);
#endif
            return frame;

        }

    }

    // If we are here then we did not find anything
    return NULL;

}






