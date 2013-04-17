#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*
 * release the backing store with ID bsid
 */
int release_bs(bsd_t bsid) {
    STATWORD ps;
    disable(ps);
    bs_free(bsid);
    restore(ps);
    return OK;
}

