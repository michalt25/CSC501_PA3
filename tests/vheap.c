/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>
#include <bs.h>
#include <frame.h>

void halt();

void task1(char *msg) {
    int i,x;
    char * str;
    char * pass = "PASS";
    char * fail = "FAIL";

    // Values for proc structure
    char pstate = 'Z';
    int pprio   = 100;
    int pesp    = 45;


    // Values for bs_map_t structure
    int pid    = 99;
    int npages = 32;
    int vpno   = 4200;

    // Values for frame_t structure
    int status = 75;
    int type   = 1;
    int refcnt = 7200;

    // Create a proc structure.. populate some print it out
    kprintf("Allocating pentry structure\n\n");
    struct pentry * pptr;
    pptr = vgetmem(sizeof(struct pentry));
    if (pptr == SYSERR) {
        kprintf("Could not allocate mem for pentry");
        return;
    }
    kprintf("Mem for pentry structure: 0x%08x\n", pptr);
    pptr->pstate = pstate;
    pptr->pprio  = pprio;
    pptr->pesp   = pesp;

    str = (pptr->pstate == pstate) ? pass : fail;
    kprintf("task1: UT1 pptr->pstate: \t%s\n", str);

    str = (pptr->pprio == pprio) ? pass : fail;
    kprintf("task1: UT2 pptr->pprio: \t%s\n", str);

    str = (pptr->pesp == pesp) ? pass : fail;
    kprintf("task1: UT3 pptr->pesp: \t%s\n", str);

    kprintf("\n\n");




    // Create a bs_map_t structure.. populate some print it out
    kprintf("Allocating bs_map_t structure\n\n");
    bs_map_t * bsmptr;
    bsmptr = vgetmem(sizeof(bs_map_t));
    if (bsmptr == SYSERR) {
        kprintf("Could not allocate mem for bs_map_t");
        return;
    }
    kprintf("Mem for bs_map_t structure: 0x%08x\n", bsmptr);
    bsmptr->pid    = pid;
    bsmptr->npages = npages;
    bsmptr->vpno   = vpno;

    str = (bsmptr->pid == pid) ? pass : fail;
    kprintf("task1: UT4 bsmptr->pid: \t%s\n", str);

    str = (bsmptr->npages == npages) ? pass : fail;
    kprintf("task1: UT5 bsmptr->npages: \t%s\n", str);

    str = (bsmptr->pid == pid) ? pass : fail;
    kprintf("task1: UT6 bsmptr->vpno: \t%s\n", str);

    kprintf("\n\n");



    // Free the proc structure
    kprintf("Freeing pentry structure\n\n");
    vfreemem((struct mblock *)pptr, sizeof(struct pentry));
    kprintf("\n\n");




    // Create a frame_t structure
    kprintf("Allocating frame_t structure\n\n");
    frame_t * frame;
    frame = vgetmem(sizeof(frame_t));
    if (frame == SYSERR) {
        kprintf("Could not allocate mem for frame_t");
        return;
    }
    kprintf("Mem for frame_t structure: 0x%08x\n", frame);
    frame->status = status;
    frame->type   = type;
    frame->refcnt = refcnt;

    str = (frame->status == status) ? pass : fail;
    kprintf("task1: UT7 frame->status: \t%s\n", str);

    str = (frame->type == type) ? pass : fail;
    kprintf("task1: UT8 frame->type: \t%s\n", str);

    str = (frame->refcnt == refcnt) ? pass : fail;
    kprintf("task1: UT9 frame->refcnt: \t%s\n", str);

    kprintf("\n\n");




    // Test the bs_map_t structure
    str = (bsmptr->pid == pid) ? pass : fail;
    kprintf("task1: UT10 bsmptr->pid: \t%s\n", str);

    str = (bsmptr->npages == npages) ? pass : fail;
    kprintf("task1: UT11 bsmptr->npages: \t%s\n", str);

    str = (bsmptr->pid == pid) ? pass : fail;
    kprintf("task1: UT12 bsmptr->vpno: \t%s\n", str);

    kprintf("\n\n");


    // Create a proc structure.. populate some print it out
    kprintf("Allocating pentry structure\n\n");
    struct pentry * pptr2;
    pptr2 = vgetmem(sizeof(struct pentry));
    if (pptr2 == SYSERR) {
        kprintf("Could not allocate mem for pentry");
        return;
    }
    kprintf("Mem for pentry structure: 0x%08x\n", pptr2);
    pptr2->pstate = pstate;
    pptr2->pprio  = pprio;
    pptr2->pesp   = pesp;

    str = (pptr2->pstate == pstate) ? pass : fail;
    kprintf("task1: UT13 pptr2->pstate: \t%s\n", str);

    str = (pptr2->pprio == pprio) ? pass : fail;
    kprintf("task1: UT14 pptr2->pprio: \t%s\n", str);

    str = (pptr2->pesp == pesp) ? pass : fail;
    kprintf("task1: UT15 pptr2->pesp: \t%s\n", str);

    kprintf("\n\n");



    // Free the bsmptr structure
    kprintf("Freeing bsmptr structure\n\n");
    vfreemem((struct mblock *)pptr, sizeof(bs_map_t));
    kprintf("\n\n");



    // Create a proc structure.. populate some print it out
    kprintf("Allocating pentry structure\n\n");
    struct pentry * pptr3;
    pptr3 = vgetmem(sizeof(struct pentry));
    if (pptr3 == SYSERR) {
        kprintf("Could not allocate mem for pentry");
        return;
    }
    kprintf("Mem for pentry structure: 0x%08x\n", pptr3);
    pptr3->pstate = pstate;
    pptr3->pprio  = pprio;
    pptr3->pesp   = pesp;

    str = (pptr3->pstate == pstate) ? pass : fail;
    kprintf("task1: UT16 pptr3->pstate: \t%s\n", str);

    str = (pptr3->pprio == pprio) ? pass : fail;
    kprintf("task1: UT17 pptr3->pprio: \t%s\n", str);

    str = (pptr3->pesp == pesp) ? pass : fail;
    kprintf("task1: UT18 pptr3->pesp: \t%s\n", str);

    kprintf("\n\n");





    // Final Tests.. So here is what we had:
    //  1. pptr =  vgetmem(sizeof(struct pentry *)
    //      -> Gets the first address (4096*NBPG)
    //  2. bsmptr = vgetmem(sizeof(bs_map_t)
    //      -> Gets addr immediately following
    //  3. vfreemem(pptr)
    //  4. frame = vgetmem(sizeof(frame_t)
    //      -> Since smaller -> Gets slot that pptr occupied
    //  5. pptr2 =  vgetmem(sizeof(struct pentry *)
    //      -> Gets address after frame
    //  6. vfreemem(bsmptr)
    //  5. pptr3 =  vgetmem(sizeof(struct pentry *)
    //      -> Since larger than bsmptr cannot occupy that space
    //      -> Gets address at end.
    //
    str = (pptr == 4096*NBPG) ? pass : fail;
    kprintf("task1: UT19 pptr mem: \t%s\n", str);

    x  = 4096*NBPG; 
    x += (int)roundmb(sizeof(struct pentry)); 
    x += (int)roundmb(sizeof(bs_map_t));
    str = (pptr2 == x) ? pass : fail;
    kprintf("task1: UT20 pptr2 mem: \t%s\n", str);

    x  = 4096*NBPG; 
    x += 2*(int)roundmb(sizeof(struct pentry)); 
    x += (int)roundmb(sizeof(bs_map_t));
    str = (pptr3 == x) ? pass : fail;
    kprintf("task1: UT21 pptr3 mem: \t%s\n", str);

    x = 4096*NBPG + sizeof(struct pentry);
    str = (bsmptr == x) ? pass : fail;
    kprintf("task1: UT22 bsmptr mem: \t%s\n", str);

    str = (frame == 4096*NBPG) ? pass : fail;
    kprintf("task1: UT23 frame mem: \t%s\n", str);


}

void vheap_test() {
    int p1;
    int hsize = 100; // heap size in pages

    kprintf("\nBasic virtual heap test\n");

    // Create a process that is always ready to run at priority 15
    p1 = vcreate(task1, 2000, 20, hsize, "task1", 0, NULL); 

    // Start the task
    kprintf("start task1 (pprio 20), then sleep 1s.\n");
    resume(p1);
    sleep(1);
    
    sleep (10);
    kprintf ("Test finished, ....\n");
}


/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main() {
////char *addr = (char*) 0x40000000; //1G
////bsd_t bs = 1;

////int i = ((unsigned long) addr) >> 12;	// the ith page

////kprintf("\n\nHello World, Xinu lives\n\n");

////get_bs(bs, 200);

////if (xmmap(i, bs, 200) == SYSERR) {
////	kprintf("xmmap call failed\n");
////	return 0;
////}

////for (i = 0; i < 16; i++) {
////	*addr = 'A' + i;
////	addr += NBPG;	//increment by one page each time
////}

////addr = (char*) 0x40000000; //1G
////for (i = 0; i < 16; i++) {
////	kprintf("0x%08x: %c\n", addr, *addr);
////	addr += 4096;       //increment by one page each time
////}

////xmunmap(0x40000000 >> 12);

    int i, s;
    int count = 0;
    char buf[8];


//  kprintf("Options are:\n");
//  kprintf("\t1 - Virtual heap test\n");
//  kprintf("\t2 - Priority Inheritance kill test\n");
//  kprintf("\t3 - Basic Priority Inheritance Test (locks)\n");
//  kprintf("\t4 - Basic Priority Inheritance Test (XINU semaphores)\n");
//  kprintf("\nPlease Input:\n");
//  while ((i = read(CONSOLE, buf, sizeof(buf))) <1);
//  buf[i] = 0;
//  s = atoi(buf);
//  switch (s) {
//  case 1:
//      // Virtual heap test
        vheap_test();
//      break;
    
 ///case 2:
 ///    // Priority Inheritance kill test
 ///    pinh_kill_test();
 ///    break;
 ///    
 ///case 3:
 ///    // Basic Priority Inheritance Test (locks)
 ///    basic_pinh_locks_test();
 ///    break;

 ///case 4:
 ///    // Basic Priority Inheritance Test (XINU semaphores)
 ///    basic_pinh_sem_test();
 ///    break;

 ///}
	return 0;
}