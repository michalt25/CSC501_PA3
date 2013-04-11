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

/*
 * init_frm_tab - initialize frm_tab
 *
 */
SYSCALL init_frmtab() {
    int i;

    for (i=0; i < NFRAMES; i++) {
        frm_tab[i].frmid  = i;        // frame id/index
        frm_tab[i].status = FRM_FREE; // Current status
        frm_tab[i].type   = FRM_FREE; // Type
       // frm_tab[i].pid  = 0;      /* process id using this frame  */
        frm_tab[i].refcnt = 0;        // reference count
        frm_tab[i].age    = 0;        // when page is loaded (in ticks)
       //frm_tab[i].dirty = 0;
      //frm_tab[i].list   = NULL;  
      //frm_tab[i].bsptr  = NULL;
        frm_tab[i].bspage = 0;
    }

    return OK;
}

//
// get_frm - get a free frame according page replacement policy.
// 
// int - returns the index of the free frame that was just reserved
//
frame_t * frm_alloc() {
    int i;



    // Iterate over the frames
    for (i=0; i < NFRAMES; i++) {

        // Is this frame free, if so use it
        if (frm_tab[i].status == FRM_FREE) {

            frm_tab[i].status = FRM_USED; // Current status
          //frm_tab[i].type   = type;     // Type
           // frm_tab[i].pid  = 0;      /* process id using this frame  */
            frm_tab[i].refcnt = 1;        // reference count
            frm_tab[i].age    = 0; //XXX  // when page is loaded (in ticks)
           //frm_tab[i].dirty = 0;

            // XXX what to do for these?
            //frm_tab[i].list   = NULL;  
            //frm_tab[i].bsptr  = NULL;
            frm_tab[i].bspage = 0;

            return &frm_tab[i];
        }
    }

    return NULL;

    // Your function to find a free frame should do the following:
    //  1. Search inverted page table for an empty frame. If one exists stop.
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


    // What to do if no free frames are available? 
    // evict one frame according to replacement policy
  kprintf("To be implemented!\n");
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
int frm_free(int frmid) {

    if (!IS_VALID_FRMID(frmid))
        return SYSERR;


    // Any other cleanup that needs to be done?
    //
    frm_tab[frmid].status = FRM_FREE;

    return OK;
}



