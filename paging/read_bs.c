#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

/*
 * fetch page page from map bs_id and write beginning 
 * at dst.
 */
SYSCALL read_bs(char *dst, bsd_t bsid, int page) {

    // Calculate the physical address of the page within
    // the backing store.
    void * phy_addr = (void *)(BS_BASE + 
                               bsid*BS_UNIT_SIZE +
                               page*NBPG);

#if DUSTYDEBUG
    kprintf("read_bs(0x%08x, %d, %d) src:0x%08x dst:0x%08x char:%c\n", 
            dst, bsid, page, 
            phy_addr, dst, 
            *(char *)phy_addr);
#endif 

    bcopy(phy_addr, (void*)dst, NBPG);
}


