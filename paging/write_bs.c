#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

/*
 *
 * write one page of data from src to the backing store 
 * bs_id, page page.
 */
SYSCALL write_bs(char *src, bsd_t bsid, int page) {


    // Calculate the physical address of the page within
    // the backing store.
    void * phy_addr = (void *)(BS_BASE + 
                               bsid*BS_UNIT_SIZE +
                               page*NBPG);
#if DUSTYDEBUG
    kprintf("write_bs(0x%08x, %d, %d) src:0x%08x dst:0x%08x char:%c\n", 
            src, bsid, page, 
            src, phy_addr, *src);
#endif 
    bcopy((void*)src, phy_addr, NBPG);

}

