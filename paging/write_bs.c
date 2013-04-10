#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

/*
 *
 * write one page of data from src to the backing store 
 * bs_id, page page.
 */
SYSCALL write_bs(char *src, bsd_t bs_id, int page) {

    // Alternate implementation
  //void * phy_addr = BS_BASE + 
  //                  bsid*BS_UNIT_SIZE +
  //                  page*NBPG;

   void * phy_addr = BS_BASE + bsid<<20 + page*NBPG;
   bcopy((void*)src, phy_addr, NBPG);

}

