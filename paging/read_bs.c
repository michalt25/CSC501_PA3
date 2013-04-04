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
SYSCALL read_bs(char *dst, bsd_t bs_id, int page) {


    // Alternate implementation
  //void * phy_addr = BACKING_STORE_BASE + 
  //                  bs_id*BACKING_STORE_UNIT_SIZE +
  //                  page*NBPG;

    void * phy_addr = BACKING_STORE_BASE + bs_id<<20 + page*NBPG;
    bcopy(phy_addr, (void*)dst, NBPG);
}


