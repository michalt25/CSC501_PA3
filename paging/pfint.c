/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>


//
// pfint - paging fault ISR
//
// A page fault indicates one of two things: the virtual page on which
// the faulted address exists is not present or the page table which
// contains the entry for the page on which the faulted address exists
// is not present.
//
SYSCALL pfint() {
    vir_addr_t vaddr; 
    pd_t * page_dir;
    pt_t * page_table;

    // Get the faulted address. The processor loads the CR2 register
    // with the 32-bit address that generated the exception.
    vaddr = (vir_addr_t) read_cr2();

    // Get the base page directory for the process
    page_dir = get_PDBR();

    // Is it a legal address (has the address been mapped) ?
    // If not then print error and kill process.
    // XXX 

    // Get the Page Table # ([31:22], upper 10 bits) which is
    //  - AKA the offset into the page directory
    vaddr.pd_offset;

    // Get the Page # ([21:12], next 10 bits)
    //  - AKA the offset into the page table
    vaddr.pt_offset;

    // Get the offset into the page ([11:0]) 
    vaddr.pg_offset;



    // If the Page Table does not exist create it.
    // XXX
    if (page_dir[pd_offset].pd_pres != 1) {

        // Create a new page table and fill in its properties in
        // the corresponding page directory entry
        page_table = new_page_table();
        if (page_table == SYSERR) {
            kprintf("Could not create page table!\n");
            return SYSERR;
        }

        page_dir[pd_offset].pd_pres  = 1;       /* page table present?      */
        page_dir[pd_offset].pd_write = 0;       /* page is writable?        */
        page_dir[pd_offset].pd_user  = 0;       /* is user level protection? */
        page_dir[pd_offset].pd_pwt   = 0;       /* write through caching for pt?*/
        page_dir[pd_offset].pd_pcd   = 0;       /* cache disable for this pt?   */
        page_dir[pd_offset].pd_acc   = 0;       /* page table was accessed? */
        page_dir[pd_offset].pd_mbz   = 0;       /* must be zero         */
        page_dir[pd_offset].pd_fmb   = 0;       /* four MB pages?       */
        page_dir[pd_offset].pd_global= 0;       /* global (ignored)     */
        page_dir[pd_offset].pd_avail = 0;       /* for programmer's use     */
        page_dir[pd_offset].pd_base  = page_table; /* location of page table?  */

    }


    // Get the address of the page table
    page_table = page_dir[pd_offset].pd_base;

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
    int backing_store;
    int page
    bsm_lookup(currpid, vaddr, &backing_store, &page);




    kprintf("To be implemented!\n");
    return OK;
}


