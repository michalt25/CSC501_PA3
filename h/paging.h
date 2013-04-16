/* paging.h */

#ifndef _PAGE_H_
#define _PAGE_H_

#include <bs.h>

#define NBPG        4096    /* number of bytes per page */


// Macro to convert virtual address into virtual page #
//
// Basically there are NBPG bytes per page which means there
// are NBPG addresses per page. 
#define VA2VPNO(vaddr) ((int)(vaddr) / NBPG)
#define VPNO2VA(vpno) ((int)(vpno)*NBPG)


// Macros used to determine the memory eviction policy
#define FIFO   3
#define AGING  4

// Structure for a page directory entry (PDE)
typedef struct {
    unsigned int pt_pres  : 1;        /* page table present?      */
    unsigned int pt_write : 1;        /* page is writable?        */
    unsigned int pt_user  : 1;        /* is use level protection? */
    unsigned int pt_pwt   : 1;        /* write through cachine for pt?*/
    unsigned int pt_pcd   : 1;        /* cache disable for this pt?   */
    unsigned int pt_acc   : 1;        /* page table was accessed? */
    unsigned int pt_mbz   : 1;        /* must be zero         */
    unsigned int pt_fmb   : 1;        /* four MB pages?       */
    unsigned int pt_global: 1;        /* global (ignored)     */
    unsigned int pt_avail : 3;        /* for programmer's use     */
    unsigned int pt_base  : 20;       /* location of page table?  */
} pd_t;


// Structure for a page table entry (PTE)
//     - Divide logical memory into blocks, called pages, of same 
//       size as physical memory frames
typedef struct {
    unsigned int p_pres  : 1;        /* page is present?     */
    unsigned int p_write : 1;        /* page is writable?        */
    unsigned int p_user  : 1;        /* is user level protection? */
    unsigned int p_pwt   : 1;        /* write through for this page? */
    unsigned int p_pcd   : 1;        /* cache disable for this page? */
    unsigned int p_acc   : 1;        /* page was accessed?       */
    unsigned int p_dirty : 1;        /* page was written?        */
    unsigned int p_mbz   : 1;        /* must be zero         */
    unsigned int p_global: 1;        /* should be zero in 586    */
    unsigned int p_avail : 3;        /* for programmer's use     */
    unsigned int p_base  : 20;       /* location of page?        */
} pt_t;


// Structure representing a virtual address
typedef struct {
    unsigned int pg_offset : 12;      /* page offset          */
    unsigned int pt_offset : 10;      /* page table offset        */
    unsigned int pd_offset : 10;      /* page directory offset    */
} virt_addr_t;



////// Structure representing a physical memory frame
//////      - Divide physical memory into ﬁxed-sized blocks 
//////        called frames 
////typedef struct {
////    int fr_status;            /* MAPPED or UNMAPPED       */
////    int fr_pid;               /* process id using this frame  */
////    int fr_vpno;              /* corresponding virtual page no*/
////    int fr_refcnt;            /* reference count      */
////    int fr_type;              /* FR_DIR, FR_TBL, FR_PAGE  */
////    int fr_dirty;
////    void *cookie;             /* private data structure   */
////    unsigned long int fr_loadtime;    /* when the page is loaded  */
////} fr_map_t;



// The first four page tables map the first 4096 pages
// (real physical memory). The information in these page tables
// is the same for all processes. We will use the following variable
// gpt (global page tables) array to keep up with these page tables.
extern pt_t * gpt[];


// pd and pt functions
int init_page_tables();
pd_t * pd_alloc();
pt_t * pt_alloc();
int pd_free(pd_t * pd);
int pt_free(pt_t * pt);
int p_invalidate(int base);


/* Prototypes for required memory API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xmunmap(int);
SYSCALL vcreate(int *, int, int, int, char *, int, long, ...);
WORD*   vgetmem(unsigned int);
SYSCALL vfreemem(struct mblock*, unsigned int);
SYSCALL srpolicy(int);
SYSCALL grpolicy();


/* given calls for dealing with backing store */
SYSCALL get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);


extern int debugTA;


#endif
