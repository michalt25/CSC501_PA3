#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*
 * release the backing store with ID bs_id
 */
SYSCALL release_bs(bsd_t bs_id) {

   return OK;
}

