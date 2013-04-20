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
#if DUSTYDEBUG
    kprintf("release_bs(%d)\n", bsid);
#endif
    bs_free(&bs_tab[bsid]);
    restore(ps);
    return OK;
}

