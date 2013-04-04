/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL int newpid();

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


    //XXX idea -
    // call "create()" to do normal stuff and then set up virtual mem
    // stuff here and then return 
    //
    //
    // set following data from process control block
    
    //  unsigned long pdbr;             /* PDBR (page directory base reg) */
    //  int     store;                  /* backing store for vheap      */
    //  int     vhpno;                  /* starting pageno for vheap    */
    //  int     vhpnpages;              /* vheap size                   */
    //  struct mblock *vmemlist;        /* vheap list                   */
    //
    //
    //
    //
    //
    //
    //  keep track of which backing store the process was allocated
    //  when it was created using vcreate(). (use int store in PCB).
    //
    //
    //  => Need to allocate a backing store for the process.
    //
    //
    //
    //
    //
    // When a process is created we must now also create page directory and
    // record the address. Also remember that the first 16 megabytes of each
    // process will be mapped to the 16 megabytes of physical memory, so we must
    // initialize the process' page directory accordingly. This is important as
    // our backing stores also depend on this correct mapping.
    //
    //
    // A mapping must be created for the new process' private heap and stack ,
    // if created with vcreate(). Because you are limited to 8 backing stores
    // you may want to use just one mapping for both the heap and the stack (as
    // with the kernel heap), vgetmem() taking from one end and the stack
    // growing from the other. (Keeping a private stack and paging it is
    // optional. But a private heap must be maintained).
    //


    // XXX From TA
    bs = get_free_bs(hsize);
    pid = create(procaddr, ssize, priority, name, nargs, args);
    ret = bs_add_mapping(BS2ID(bs), pid, 4096, hsize);
    /*Initialize the memlist here.*/
    proc->memlist.mnext = ...

    kprintf("To be implemented!\n");
    return OK;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *
 * XXX may not need this 
 *------------------------------------------------------------------------
 */
LOCAL int newpid() {
    int pid;            /* process id to return     */
    int i;

    for (i=0 ; i<NPROC ; i++) { /* check all NPROC slots    */
        if ( (pid=nextproc--) <= 0)
            nextproc = NPROC-1;
        if (proctab[pid].pstate == PRFREE)
            return(pid);
    }
    return(SYSERR);
}
