/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
    STATWORD ps;    
    struct  pentry  *pptr;      /* points to proc. table for pid*/
    int dev;

    disable(ps);
    if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
        restore(ps);
        return(SYSERR);
    }
    if (--numproc == 0)
        xdone();

    // 
    // When a process dies the following should happen.
    // 1. All frames which currently hold any of its pages should be written to
    //    the backing store and be freed.
    // 2. All of it's mappings should be removed from the backing store map.
    // 3. The backing stores for its heap (and stack if have chosen to implement
    //    a private stack) should be released (remember backing stores
    //    allocated by a process should persist unless the process explicitly
    //    releases them).
    // 4. The frame used for the process' page directory should be released.


    dev = pptr->pdevs[0];
    if (! isbaddev(dev) )
        close(dev);
    dev = pptr->pdevs[1];
    if (! isbaddev(dev) )
        close(dev);
    dev = pptr->ppagedev;
    if (! isbaddev(dev) )
        close(dev);
    
    send(pptr->pnxtkin, pid);

    freestk(pptr->pbase, pptr->pstklen);
    switch (pptr->pstate) {

    case PRCURR:    pptr->pstate = PRFREE;  /* suicide */
            resched();

    case PRWAIT:    semaph[pptr->psem].semcnt++;

    case PRREADY:   dequeue(pid);
            pptr->pstate = PRFREE;
            break;

    case PRSLEEP:
    case PRTRECV:   unsleep(pid);
                        /* fall through */
    default:    pptr->pstate = PRFREE;
    }
    restore(ps);
    return(OK);
}
