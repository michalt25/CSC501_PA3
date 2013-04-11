#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

/*
 * fetch page page from map bs_id and write beginning 
 * at dst.
 */
SYSCALL read_bs(char *dst, bsd_t bsid, int page) {


    // Alternate implementation
  //void * phy_addr = (void *)(BS_BASE + 
  //                           bsid*BS_UNIT_SIZE +
  //                           page*NBPG);

    void * phy_addr = (void *)(BS_BASE + bsid<<20 + page*NBPG);
    bcopy(phy_addr, (void*)dst, NBPG);
}


