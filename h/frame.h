// frame.h 
#ifndef _FRAME_H_
#define _FRAME_H_

#define FRAME0      1024    /* zero-th frame        */
#define NFRAMES     1024    /* number of frames     */

// Macro to convert frame ID/INDEX to physical mem address
#define FID2PA(frmid)   ((FRAME0 + (frmid))*NBPG)
#define FID2VPNO(frmid) (FRAME0 + (frmid))
#define PA2FID(addr)    (((addr)/NBPG) - FRAME0)


//XXX investigate these
//#define FP2FN(frm)  (((frm) - frm_tab) + FRAME0)
//#define FN2ID(fn)   ((fn) - FRAME0)
//#define FP2PA(frm)  ((void*)(FP2FN(frm) * NBPG))


// Macro to verify the given frmid is valid
#define IS_VALID_FRMID(frmid) (((frmid) < NFRAMES) && ((frmid) >= 0))


// Macro to figure out how many Page directory entires can fit 
// in a single frame of physical memory:
//
// Note: Each page directory entry (pd_t) is 32 bits (4 bytes),
//       while a frame of memory is NBPG bytes. 
//
//       Therefore 1 frame can hold NBPG/4 entries. For a NBPG of
//       4096 this is:
//
//           4096/4 = 1024 total entries in page directory
//
//       => 1 frame of mem can store 1024 page directory entries
//
#define NENTRIES (NBPG/4)

// Macros to determine if a memory frame is 
// mapped or not.
//#define FRM_UNMAPPED 0
//#define FRM_MAPPED   1

// Macros to determine the frame type. The types are:
//     FR_PAGE - contains actual logical memory
//     FR_TBL  - contains a page table
//     FR_DIR  - contains a page table directory
//#define FRM_PAGE 0
//#define FRM_TBL  1
//#define FRM_DIR  2


// Macros to determine the frame status.
#define FRM_FREE 0
#define FRM_USED 1

// Macros to determine the frame type. The types are:
//     FRM_PD - contains a page table directory
//     FRM_PT - contains a page table
//     FRM_BS - Part of a backing store  
#define FRM_PD 0
#define FRM_PT 1
#define FRM_BS 2


typedef struct _frame_t {
    int frmid;  // The frame id (index) 
    int status; // FRM_FREE - frame is not being used
                // FRM_USED - frame is being used

    int type;   // FRM_PD   - Being used for a page directory
                // FRM_PT   - Being used for a page table
                // FRM_BS   - Part of a backing store

    int refcnt; // If the frame is used for a page table (FRM_PGT),
                // refcnt is the number of mappings in the table. 
                // When refcnt is 0 release the frame. 
                //
                // If the frame is used for FRM_BS, refcnt will be the
                // number of times this frame is mapped by processes.
                // When refcnt is 0 release the frame.

    int age; // when the page is loaded, in ticks
             // Used for page replacement policy AGING 
                

    struct _frame_t * next; // was originally called fifo (may be a clue)
        // XXX The following is a guess as to what this is needed for:
        // Pages may belong to many lists.. lists of free frames, dirty
        // frames, clean frames, etc.. 


    // Following are only used if status == FRM_BS
////bs_t * bsptr;              // The backing store this frame maps to
    int    bsid;              // The backing store this frame maps to
    int    bspage;            // The page within the backing store
   // XXX don't know if i need this struct _frame_t * bs_next; // The list of all the frames for this bs

} frame_t;


frame_t * frm_alloc(); 

// Table with entries representing frame
extern frame_t frm_tab[];


#endif
