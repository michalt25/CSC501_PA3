/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>



// When writing out a dirty page the only way to figure out which backing store
// a dirty frame belongs to would be to traverse the page tables of every process 
// looking for a frame location that corresponds to the frame we wish to write out. 
// This is inefficient. To prevent this we use an inverted page table (maps frames 
// to pages) named frm_tab that holds NFRAMES entries and each entry is a frame_t 
// structure that contains the following data in each :
//
//  int frmid;  // The frame id (index) 
//  int status; // FRM_FREE - frame is not being used
//              // FRM_USED - frame is being used
//  int type;   // FRM_PD   - Being used for a page directory
//              // FRM_PT   - Being used for a page table
//              // FRM_BS   - Part of a backing store
//  int accessed; // Was the frame accessed or not since last
//                // check?
//  int refcnt; // If the frame is used for a page table (FRM_PGT),
//              // refcnt is the number of mappings in the table. 
//              // When refcnt is 0 release the frame. 
//              //
//              // If the frame is used for FRM_BS, refcnt will be the
//              // number of times this frame is mapped by processes.
//              // When refcnt is 0 release the frame.
//  int age; // when the page is loaded, in ticks
//           // Used for page replacement policy AGING 
//              
//  struct _frame_t * fifo_next;
//      // The fifo that keeps up with the order in which frames were
//      // allocated. Oldest frames are at the head of the fifo.
//  // Following are only used if status == FRM_BS
//  int    bsid;              // The backing store this frame maps to
//  int    bspage;            // The page within the backing store
//  struct _frame_t * bs_next; // The list of all the frames for this bs
//
//
// With this information we can easily find what bs and bspage the physical frame
// maps to and write it to the appropriate location.
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
 * _frm_cleanlists - Go through the lists and clean up frames that are no
 *                   longer in use. Only need to provide head of
 *                   bs_next list.. frm_fifo_head is global. If NULL is
 *                   passed in for bs_next_head then we won't go
 *                   through that list.
 */
int _frm_cleanlists(void * bspointer) {
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
            kprintf("_frm_cleanlists(): removing frm %d from fifo_next list\n", 
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
            kprintf("_frm_cleanlists(): removing frm %d from bs_next list\n", 
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
        _frm_cleanlists(&bs_tab[frame->bsid]);
    else
        _frm_cleanlists(NULL);


    // Any other cleanup that needs to be done..
    //
    // Note: Don't clean up bs_next and fifo_next 
    //       as loops may still be using them for now.
    //       They will be set to null in frm_alloc()
    frame->type   = FRM_FREE;
    frame->refcnt = 0;
    frame->age    = 0;
    frame->bsid   = -1;
    frame->bspage = 0;
    frame->accessed = 0;


    return OK;
}

/*
 * _frm_evict - Find a frame to evict from memory
 */
frame_t * _frm_evict() {
    int i;
    frame_t * frame;

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
        frm_tab[i].refcnt = 0;        // reference count
        frm_tab[i].age    = 0;        // when page is loaded (in ticks)
        frm_tab[i].bspage = 0;
        frm_tab[i].bsid   = -1;
        frm_tab[i].accessed  = 0;
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
    frame->status    = FRM_USED; // Current status
    frame->refcnt    = 0;        // should be updated by caller
    frame->accessed  = 0;
    frame->age       = 0;
    frame->bsid      = -1;
    frame->bspage    = 0;
    frame->fifo_next = NULL;
    frame->bs_next   = NULL;

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


/*
 *  frm_update_ages - function to update frame ages. Called
 *                    during a page fault.
 */
int frm_update_ages() {
    int proc, i, j, x;
    struct pentry * pptr;
    pd_t * pd;
    pt_t * pt;
    frame_t * frame;
    frame_t * curr;


    for (proc=0; proc<NPROC; proc++) {

        // If this proc doesn't exist skip
        pptr = &proctab[proc];
        if (pptr->pstate == PRFREE)
            continue;

        // Get the page dir for this process
        // and iterate over entries
        //
        // Note: Must start at 4 because we don't 
        // want to operate on entries from first 4 
        // page tables.
        pd = pptr->pd; 
        for (i=4; i<NENTRIES; i++) {

            // Is this page table present?
            if (pd[i].pt_pres) {
                pt = VPNO2VA(pd[i].pt_base);

                // Iterate over page table entries
                for (j=0; j<NENTRIES; j++) {

                    // Has it been accessed?
                    if (pt[j].p_pres && pt[j].p_acc) {

                        frame = PA2FP(VPNO2VA(pt[j].p_base));
                        frame->accessed = 1;
                        pt[j].p_acc = 0; // reset accessed bit

                    }

                }

            }

        }

    }

    // Now that all the accessed bits are updated lets update
    // the ages of the frames. We needed to do this separately because 
    // otherwise if two pages map to a single frame the frames would
    // get updated twice everytime this was done. By updating the
    // accessed bit in the frame (doesn't matter if you update it
    // twice) first, then we ensure we only update the age once.
    curr = frm_fifo_head;
    while (curr) {

        x = curr->age;

        // All frames age get decreased by half
        // Keep in mind we evict pages with smallest
        // age (decreasing age increases chance of 
        // page replacement).
        curr->age = curr->age >> 1;

        // If the page was accessed then we add 128 to the age
        if (curr->accessed) {

            curr->age += 128;
            if (curr->age > 255)
                curr->age = 255; //max out at 255

            curr->accessed = 0; // reset access flag
        }

#if DUSTYDEBUG
        // Print out message if age changed
        if (x != curr->age)
            kprintf("Updated age of frame %d from %d to %d %s\n",
                    curr->frmid, x, curr->age,
                    (curr->age > x) ? "(accessed)" : "");
#endif

        // Go to next frame in list
        curr = curr->fifo_next;
    }

    return OK;
}
