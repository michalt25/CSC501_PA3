/* initialize.c - nulluser, sizmem, sysinit */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <sem.h>
#include <sleep.h>
#include <mem.h>
#include <tty.h>
#include <q.h>
#include <io.h>
#include <paging.h>
#include <bs.h>
#include <frame.h>

/*#define DETAIL */
#define HOLESIZE    (600)   
#define HOLESTART   (640 * 1024)
#define HOLEEND     ((1024 + HOLESIZE) * 1024)  
/* Extra 600 for bootp loading, and monitor */

extern  int main(); /* address of user's main prog  */

extern  int start();

LOCAL       sysinit();

/* Declarations of major kernel variables */
struct  pentry  proctab[NPROC]; /* process table            */
int nextproc;                   /* next process slot to use in create   */
struct  sentry  semaph[NSEM];   /* semaphore table          */
int nextsem;                    /* next sempahore slot to use in screate*/
struct  qent    q[NQENT];       /* q table (see queue.c)        */
int nextqueue;                  /* next slot in q structure to use  */
char    *maxaddr;               /* max memory address (set by sizmem)   */
struct  mblock  memlist;        /* list of free memory blocks       */
#ifdef  Ntty
struct  tty     tty[Ntty];      /* SLU buffers and mode control     */
#endif

/* active system status */
int numproc;        /* number of live user processes    */
int currpid;        /* id of currently running process  */
int reboot = 0;     /* non-zero after first boot        */

int rdyhead,rdytail;    /* head/tail of ready list (q indicies) */
char    vers[80];
int console_dev;        /* the console device           */

/*  added for the demand paging */
int page_replace_policy = FIFO;
int debugTA = 0;

/************************************************************************/
/***                NOTE:                     ***/
/***                                      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED, and      ***/
/***   must eventually be enabled explicitly.  This routine turns     ***/
/***   itself into the null process after initialization.  Because    ***/
/***   the null process must always remain ready to run, it cannot    ***/
/***   execute code that might cause it to be suspended, wait for a   ***/
/***   semaphore, or put to sleep, or exit.  In particular, it must   ***/
/***   not do I/O unless it uses kprintf for polled output.           ***/
/***                                      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  nulluser  -- initialize system and become the null process (id==0)
 *------------------------------------------------------------------------
 */
nulluser()              /* babysit CPU when no one is home */
{
        int userpid;

    console_dev = SERIAL0;      /* set console to COM0 */

    initevec();

    kprintf("system running up!\n");
    sysinit();

    enable();       /* enable interrupts */

    sprintf(vers, "PC Xinu %s", VERSION);
    kprintf("\n\n%s\n", vers);
    if (reboot++ < 1)
        kprintf("\n");
    else
        kprintf("   (reboot %d)\n", reboot);


    kprintf("%d bytes real mem\n",
        (unsigned long) maxaddr+1);
#ifdef DETAIL   
    kprintf("    %d", (unsigned long) 0);
    kprintf(" to %d\n", (unsigned long) (maxaddr) );
#endif  

    kprintf("%d bytes Xinu code\n",
        (unsigned long) ((unsigned long) &end - (unsigned long) start));
#ifdef DETAIL   
    kprintf("    %d", (unsigned long) start);
    kprintf(" to %d\n", (unsigned long) &end );
#endif

#ifdef DETAIL   
    kprintf("%d bytes user stack/heap space\n",
        (unsigned long) ((unsigned long) maxaddr - (unsigned long) &end));
    kprintf("    %d", (unsigned long) &end);
    kprintf(" to %d\n", (unsigned long) maxaddr);
#endif  
    
    kprintf("clock %sabled\n", clkruns == 1?"en":"dis");


    /* create a process to execute the user's main program */
    userpid = create(main,INITSTK,INITPRIO,INITNAME,INITARGS);
    resume(userpid);

    while (TRUE)
        /* empty */;
}

/*------------------------------------------------------------------------
 *  sysinit  --  initialize all Xinu data structeres and devices
 *------------------------------------------------------------------------
 */
LOCAL
sysinit()
{
    static  long    currsp;
    int i,j;
    struct  pentry  *pptr;
    struct  sentry  *sptr;
    struct  mblock  *mptr;
    SYSCALL pfintr();
    int rc;
    pd_t * pd;

    

    numproc = 0;            /* initialize system variables */
    nextproc = NPROC-1;
    nextsem = NSEM-1;
    nextqueue = NPROC;      /* q[0..NPROC-1] are processes */


    kprintf("here1\n");

    /* initialize free memory list */
    /* PC version has to pre-allocate 640K-1024K "hole" */
    if (((unsigned int)maxaddr+1) > HOLESTART) {

        // There is a "HOLE" in our free memory section that is
        // reserved for the PC usage. On either side of this HOLE
        // there is free memory available. Our list of free memory
        // will initially consist of two "blocks". The block before
        // the HOLE and the block after the HOLE.

        // Get the start of the first block of free memory. We can find 
        // this by using the "end" variable (represents first address past 
        // the end of the uninitialized data segment).
        memlist.mnext = mptr = (struct mblock *) roundmb(&end);

        // The next memory block will start at the end of the HOLE 
        mptr->mnext = (struct mblock *)HOLEEND;

        // The size of the first memory block is the size of the space
        // between the end of the unitialized data segment and the
        // start of the HOLE. 
        mptr->mlen = (int) truncew(((unsigned) HOLESTART -
                 (unsigned)&end));
        mptr->mlen -= 4;

        // Calculate the size of the next block of memory 
        mptr = (struct mblock *) HOLEEND;
        mptr->mnext = 0;
        mptr->mlen = (int) truncew((unsigned)maxaddr - HOLEEND -
                NULLSTK);
/*
        mptr->mlen = (int) truncew((unsigned)maxaddr - (4096 - 1024 ) *  4096 - HOLEEND - NULLSTK);
*/
    } else {
        /* initialize free memory list */
        memlist.mnext = mptr = (struct mblock *) roundmb(&end);
        mptr->mnext = 0;
        mptr->mlen = (int) truncew((unsigned)maxaddr - (int)&end -
            NULLSTK);
    }

    kprintf("here2\n");
// The NULL process is somewhat of a special case, as it builds itself
// in the function sysinit(). The NULL process should not have a
// private heap(like any process created with create()).
// The following should occur at system initialization:
//


    // Initialize all necessary data structures.
    // Create the page tables which will map pages 0 through 4095 to the
    // physical 16 MB. These will be called the global page tables.
    // Allocate and initialize a page directory for the NULL process.
    // Set the PDBR register to the page directory for the NULL process.
    // Install the page fault interrupt service routine.
    // Enable paging.
    //

    //
    // 1 - inititalize (zero out values)
    //      - backing store - init_bsm()
    //      - frames        - init_frm()
    //      - install page fault handler
    // 2 - create new page table for null process
    //      - create page directory (outer page table)
    //      - initialize 1:1 mapping for first 4096 pages
    //          - allocate 4 pages tables (4*1024 pages)
    //          - assign each page table entry to the address starting
    //            from page number 0 to 1023
    //
    // All memory below page 4096 will be "global". That is, it is
    // usable and visible by all processes and accessible by simply
    // using its actual physical
    // addresses. As a result, the first four page tables for every
    // process will be the same, and thus should be shared.
    //
    //
    // 3 - Enable paging
    //      - set bit 31 of CR0 register
    //      - take care that PDBR is set because subsequent memory
    //        address access will be virtual memory address
    // 4 - Creating new process (main)
    //      - create page directory (same as with null process)
    //      - share the first 4096 pages with null process (since they
    //        were already created at system initialization time).
    // 5 - Context switch
    //      - every process has separate page directory
    //      - before ctxsw() load CR3 with the process' PDBR


    // Zero out values of backing stores
    rc = init_bstab();
    if (rc == SYSERR)
        return SYSERR;

    // Zero out values of frames
    rc = init_frmtab();
    if (rc == SYSERR)
        return SYSERR;
    

    kprintf("here3\n");
    // Create and initialize the first four page tables. These map to
    // the first 4096 of memeory (physical memory) and are shared
    // among all processes.
    rc = init_page_tables();
    if (rc == SYSERR)
        return SYSERR;

    kprintf("here4\n");
    // Create a page directory for the null process
    pd = pd_alloc();
    if (pd == NULL)
        return SYSERR;

    kprintf("here5\n");
    // Set PDBR register (part of CR3) with value for PDBR of null
    // process
    set_PDBR(VA2VPNO(pd));

    kprintf("here6\n");
    // Install the page fault interrupt service routine.
    // XXX
    set_evec(14, (unsigned long) pfintr);

    kprintf("here7\n");
    // Enable paging (set bit 31 of CR0 register)
    enable_paging();


    







    for (i=0 ; i<NPROC ; i++)   /* initialize process table */
        proctab[i].pstate = PRFREE;


#ifdef  MEMMARK
    _mkinit();          /* initialize memory marking */
#endif

#ifdef  RTCLOCK
    clkinit();          /* initialize r.t.clock */
#endif

    mon_init();     /* init monitor */

#ifdef NDEVS
    for (i=0 ; i<NDEVS ; i++ ) {        
        init_dev(i);
    }
#endif

    pptr = &proctab[NULLPROC];  /* initialize null process entry */
    pptr->pstate = PRCURR;
    for (j=0; j<7; j++)
        pptr->pname[j] = "prnull"[j];
    pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK;
    pptr->pbase = (WORD) maxaddr - 3;
/*
    pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK - (4096 - 1024 )*4096;
    pptr->pbase = (WORD) maxaddr - 3 - (4096-1024)*4096;
*/
    pptr->pesp = pptr->pbase-4; /* for stkchk; rewritten before used */
    *( (int *)pptr->pbase ) = MAGIC;
    pptr->paddr = (WORD) nulluser;
    pptr->pargs = 0;
    pptr->pprio = 0;

    pptr->pd = pd; // Set the pdbr for the null proc


    currpid = NULLPROC;

    for (i=0 ; i<NSEM ; i++) {  /* initialize semaphores */
        (sptr = &semaph[i])->sstate = SFREE;
        sptr->sqtail = 1 + (sptr->sqhead = newqueue());
    }

    rdytail = 1 + (rdyhead=newqueue());/* initialize ready list */
    kprintf("here8\n");


    return(OK);
}

stop(s)
char    *s;
{
    kprintf("%s\n", s);
    kprintf("looping... press reset\n");
    while(1)
        /* empty */;
}

delay(n)
int n;
{
    DELAY(n);
}


#define NBPG    4096

/*------------------------------------------------------------------------
 * sizmem - return memory size (in pages)
 *------------------------------------------------------------------------
 */
long sizmem()
{
    unsigned char   *ptr, *start, stmp, tmp;
    int     npages;

    /* at least now its hacked to return
       the right value for the Xinu lab backends (16 MB) */

    return 4096; 

    start = ptr = 0;
    npages = 0;
    stmp = *start;
    while (1) {
        tmp = *ptr;
        *ptr = 0xA5;
        if (*ptr != 0xA5)
            break;
        *ptr = tmp;
        ++npages;
        ptr += NBPG;
        if ((int)ptr == HOLESTART) {    /* skip I/O pages */
            npages += (1024-640)/4;
            ptr = (unsigned char *)HOLEEND;
        }
    }
    return npages;
}
