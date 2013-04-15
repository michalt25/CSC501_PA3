/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
 * vcreate  - This call will create a new Xinu process. The difference
 * from create() is that the process's heap will be private and exist
 * in its virtual memory. The size of the heap (in number of pages) is 
 * specified by the user.
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
    int *procaddr;  /* procedure address           */
    int  ssize;     /* stack size in words         */
    int  hsize;     /* virtual heap size in pages  */
    int  priority;  /* process priority > 0        */
    char *name;     /* name (for debugging)        */
    int  nargs;     /* number of args that follow  */
    long args;      /* arguments (treated like an array in the code) */
{

    STATWORD ps;    
    int rc;
    int pid;
    bs_t * bsptr;
    struct pentry * pptr;
    struct mblock * memblock;

    // Disable interrupts
    disable(ps);

    // Perform normal process stuff! 
    pid = create(procaddr, ssize, priority, name, nargs, args);
    pptr = &proctab[pid];


    // Get a pointer to a free backing store
    bsptr = get_free_bs(hsize);
    if (bsptr == NULL) {
        kprintf("vcreate(): could not find free backing store\n");
        restore(ps);
        return SYSERR;
    }

    // populate the backing store with information
    bsptr->status = BS_USED;
    bsptr->isheap = 1;
    bsptr->npages = hsize;
    bsptr->maps   = NULL;
    bsptr->frames = NULL;


    // Add a mapping between this process and this backing 
    // store. The vpno starts at 4096 (the first possible virtual
    // page for this process).
    rc = bs_add_mapping(bsptr->bsid, pid, 4096, hsize);
    if (rc == SYSERR) {
        kprintf("vcreate(): could not add mapping\n");
        restore(ps);
        return SYSERR;
    }

    // Populate some info in the PCB
    pptr->bsid  = bsptr->bsid;
    pptr->hvpno = 4096;  
    pptr->hsize = hsize;


    // Initialize the free memory list for the virtual heap
    //
    // The first item in the list will be the addr that maps 
    // to 1st page of the backing store
    pptr->vmemlist.mnext = (struct mblock *) (4096*NBPG);

    // Since this process hasn't started to run yet page fault won't
    // work. We need to actually write mnext and mlen directly to the
    // first page of the backing store.
    memblock = BSID2PA(bsptr->bsid);
    memblock->mnext = 0;  
    memblock->mlen  = hsize*NBPG;

    restore(ps);
    return pid;
}
