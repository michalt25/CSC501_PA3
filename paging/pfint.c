/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <paging.h>
#include <control_reg.h>



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
    int bsoffset;
    bs_t * bsptr;
    bs_map_t * bsmptr;
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
    kprintf("!PAGE FAULT for address 0x%08x\tprocess %d\n", cr2, currpid);
#endif

    // Update the ages for all frames
    if (grpolicy() == AGING)
        frm_update_ages();


    // Get the base page directory for the process
    pptr = &proctab[currpid];
    pd   = pptr->pd;

    // Is it a legal address (has the address been mapped) ?
    // Use backing store map to find the store and page offset
    bsmptr = bs_lookup_mapping(currpid, VA2VPNO(cr2));
    if (bsmptr == NULL) {
        kprintf("pfint(): could not find mapping!\n");
        goto error;
    }

    // Get the page offset of the frame from beginning of bs
    bsoffset = VA2VPNO(cr2) - bsmptr->vpno;

    // Get a pointer to the bs_t structure for the backing store
    bsptr = &bs_tab[bsmptr->bsid];

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

    // Check to see if the page from the backing store is already in 
    // physical memory (a frame). If not we will have to allocate a 
    // new frame and bring the data in from disk (backing store).
    frame = frm_find_bspage(bsptr->bsid, bsoffset);
    if (frame == NULL) {

        // Get a free frame
        frame = frm_alloc();
        if (frame == NULL) {
            kprintf("pfint(): could not get free frame!\n");
            goto error;
        }

        // Populate a little more information in the frame
        frame->type   = FRM_BS;
        frame->bsid   = bsptr->bsid;
        frame->bspage = bsoffset;
        frame->refcnt = 1;

        // Add the frame to head of the frame list within the bs_t struct
        frame->bs_next = bsptr->frames;
        bsptr->frames = frame;

        // Copy the page from the backing store into the frame
        read_bs((void *)FID2PA(frame->frmid), bsmptr->bsid, bsoffset);

    } else {
        frame->refcnt++;
    }

    // Update the page table
    pt[pt_offset].p_pres  = 1;
    pt[pt_offset].p_write = 1;
    pt[pt_offset].p_base  = FID2VPNO(frame->frmid);

    // Increase the refcount in the page table's frame
    ptframe = PA2FP(pt);
    ptframe->refcnt++;

    // Finally must invalidate TLB entries since page table contents 
    // have changed. From intel vol III
    //
    // All of the (nonglobal) TLBs are automatically invalidated any
    // time the CR3 register is loaded.
    set_PDBR(VA2VPNO(pd));


    restore(ps);
    return OK;


error:
    kill(currpid);
    restore(ps);
    return SYSERR;
}
