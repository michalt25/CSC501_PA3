/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <paging.h>
#include <control_reg.h>




//XXX should interrupts are disabled during page allocation? Are they
//    already while we are in here?


/*
 * pfint - paging fault ISR
 *
 * A page fault indicates one of two things: the virtual page on which
 * the faulted address exists is not present or the page table which
 * contains the entry for the page on which the faulted address exists
 * is not present.
 */
SYSCALL pfint() {
    STATWORD ps;    
    virt_addr_t * vaddr; 
    pd_t * pd;
    pt_t * pt;
    int rc;
    int pd_offset;
    int pt_offset;
    int pg_offset;
    bsd_t bsid;
    bs_t * bsptr;
    int bsoffset;
    frame_t * frame;
    frame_t * ptframe;
    unsigned long cr2;
    struct pentry * pptr;

    // Disable interrupts
    disable(ps);

    // Get the faulted address. The processor loads the CR2 register
    // with the 32-bit address that generated the exception.
    //vaddr = (virt_addr_t) read_cr2();
    cr2 = read_cr2();
    vaddr = (virt_addr_t *)(&cr2);


#if DUSTYDEBUG
    kprintf("Page Fault for address 0x%08x\tprocess %d\n", cr2, currpid);
#endif

    // Get the base page directory for the process
    pptr = &proctab[currpid];
    pd   = pptr->pd;

    // Is it a legal address (has the address been mapped) ?
    // Use backing store map to find the store and page offset
    rc = bs_lookup_mapping(currpid, VA2VPNO(cr2), &bsid, &bsoffset);
    if (rc == SYSERR) {
        kprintf("pfint(): could not find mapping!\n");
        goto error;
    }

    // Get a pointer to the bs_t structure for the backing store
    bsptr = &bs_tab[bsid];

    // Get the Page Table # ([31:22], upper 10 bits) which is
    //  - AKA the offset into the page directory
    pd_offset = vaddr->pd_offset;

    // Get the Page # ([21:12], next 10 bits)
    //  - AKA the offset into the page table
    pt_offset = vaddr->pt_offset;

    // Get the offset into the page ([11:0]) 
    pg_offset = vaddr->pg_offset;



    // If the Page Table does not exist create it.
    if (pd[pd_offset].pt_pres != 1) {

        // Create a new page table and fill in its properties in
        // the corresponding page directory entry
        pt = pt_alloc();
        if (pt == NULL) {
            kprintf("Could not create page table!\n");
            goto error;
        }

        pd[pd_offset].pt_pres  = 1;   /* page table present?      */
        pd[pd_offset].pt_write = 1;   /* page is writable?        */
        pd[pd_offset].pt_user  = 0;   /* is user level protection? */
        pd[pd_offset].pt_pwt   = 0;   /* write through caching for pt?*/
        pd[pd_offset].pt_pcd   = 0;   /* cache disable for this pt?   */
        pd[pd_offset].pt_acc   = 0;   /* page table was accessed? */
        pd[pd_offset].pt_mbz   = 0;   /* must be zero         */
        pd[pd_offset].pt_fmb   = 0;   /* four MB pages?       */
        pd[pd_offset].pt_global= 0;   /* global (ignored)     */
        pd[pd_offset].pt_avail = 0;   /* for programmer's use     */
        pd[pd_offset].pt_base  = VA2VPNO(pt);  /* Page # where pt is located */

    }

    // Get the address of the page table
    pt = VPNO2VA(pd[pd_offset].pt_base);

  //// Check to see if the page from the backing store
  //// is already in physical memory. If so then bump
  //// the refcnt and continue
  //if (PA2FP(


    // Get a free frame
    frame = frm_alloc();
    if (frame == NULL) {
        kprintf("pfint(): could not get free frame!\n");
        goto error;
    }

    // Populate a little more information in the frame
    frame->pte    = &pt[pt_offset];
    frame->type   = FRM_BS;
    frame->bsid   = bsptr->bsid;
    frame->bspage = bsoffset;

    // Add the frame to head of the frame list within the bs_t struct
    frame->bs_next = bsptr->frames;
    bsptr->frames = frame;

    // Copy the page from the backing store into the frame
    read_bs((void *)FID2PA(frame->frmid), bsid, bsoffset);

    // Update the page table
    pt[pt_offset].p_pres  = 1;
    pt[pt_offset].p_write = 1;
    pt[pt_offset].p_base  = FID2VPNO(frame->frmid);

    // Increase the refcount in the page table's frame
  //PA2FP(pt)->refcnt++;
  //ptframe = &frm_tab[PA2FID(pt)];
    ptframe = PA2FP(pt);
    ptframe->refcnt++;

    // Finally must invalidate TLB entries since page table contents 
    // have changed. From intel vol III
    //
    // All of the (nonglobal) TLBs are automatically invalidated any
    // time the CR3 register is loaded.
    set_PDBR(VA2VPNO(pd));



    // Increment reference count in inverted page table

    // To page in a faulted page do:
    //      - Use backing store map to find the store and page offset
    //      - Increment ref count in inverted page table
    //      - Get a free frame
    //      - Copy that page from backing store into frame
    //      - Update page table 
    //          - mark as present
    //          - update address portion to point to new frame
    //          - any other needed items
    // XXX
////int backing_store;
////int page
////bsm_lookup(currpid, vaddr, &backing_store, &page);



////// In xmmap(), you will need to set up the mapping; e.g., 
////// int bs_add_mapping(bs_id, currpid, vpno, npages). Once you 
////// have a mapping, when a page fault takes place, you can search 
////// bs_id and pagth through the map. Now you can allocate a new 
////// frame for this backstore; e.g., frame_t *bs_get_frame(bs_id, pagth).
////bs_get_frame();V


    restore(ps);
    return OK;

error:
    kill(currpid);
    restore(ps);
    return SYSERR;
}


