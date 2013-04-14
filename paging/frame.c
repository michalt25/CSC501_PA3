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


/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
int frm_free(frame_t * frame) {

    bs_t * bsptr;
    pt_t * pte;

    if (!IS_VALID_FRMID(frame->frmid))
        return SYSERR;

#if DUSTYDEBUG
    kprintf("frm_free(): Freeing frame %d\n", frame->frmid);
#endif 


    // If this frame is mapped from a backing store
    // then write the data back to the backing store
    if (frame->type == FRM_BS) {

        // Make sure the bsid is valid
        if (!IS_VALID_BSID(frame->bsid))
            return SYSERR;
        
        write_bs(FID2PA(frame->frmid), frame->bsid, frame->bspage);

    }

    // Invalidate the page table entry for this frame
    if (frame->pte) {
        pte = (pt_t *)frame->pte;
        pte->p_pres = 0;
    }

    // Any other cleanup that needs to be done?
    //
    frame->status = FRM_FREE;

    return OK;
}

/*-------------------------------------------------------------------------
 * _frm_evict - Find a frame to evict from memory
 *-------------------------------------------------------------------------
 */
frame_t * _frm_evict() {
    int i;
    frame_t * frame;
    frame_t * prev;
    frame_t * curr;


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

        // Is this frames refcnt 0? If so use it
        if (frame->refcnt == 0) {
            frm_free(frame);
            return frame;
        }

    }

#if DUSTYDEBUG
    kprintf("_frm_evict(): Evicting frame using FIFO replacement\n");
#endif 

    // If we made it here then we found no candidates and 
    // we must force one page out of memory to free up space.
    //
    // Release the frame closest to the head of the fifo that 
    // isn't a page directory or page table
    prev = NULL;
    curr = frm_fifo_head;
    while (curr) {
        if (curr->type == FRM_BS) {
            prev->fifo_next = curr->fifo_next;
            frm_free(curr);
            return curr;
        }
        prev = curr;
        curr = curr->fifo_next;
    }

    // If we made it here then there was a problem
    return NULL;
}

/*
 * init_frm_tab - initialize frm_tab
 *
 */
SYSCALL init_frmtab() {
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
  //frm_tab[i].type   = type;     // Type
   // frm_tab[i].pid  = 0;      /* process id using this frame  */
    frame->refcnt = 1;        // reference count
    frame->age    = 0; //XXX  // when page is loaded (in ticks)
   //frm_tab[i].dirty = 0;
    frame->bsid   = -1;
    frame->bspage = 0;
    frame->fifo_next = NULL;
    frame->bs_next = NULL;
    frame->pte    = NULL;

    // Add frame to end of fifo. 
    if (frm_fifo_head == NULL) {
        frm_fifo_head = frame;
    } else {
        curr = frm_fifo_head;
        while (curr->fifo_next != NULL)
            curr = curr->fifo_next;
        curr->fifo_next = frame;
    }
  
  //frame->fifo_next = frm_fifo_head;
  //frm_fifo_head = frame;

    return frame;

}






