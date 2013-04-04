/* paging.h */

#define NBPG        4096    /* number of bytes per page */
#define FRAME0      1024    /* zero-th frame        */
#define NFRAMES     1024    /* number of frames     */


// Macro to convert virtual address into virtual page #
//
// Basically there are NBPG bytes per page which means there
// are NBPG addresses per page. 
#define VADDR_TO_VPNO(vaddr) ((vaddr)/NBPG)
#define VPNO_TO_VADDR(vpno) ((vpno)*NBPG)

// Macro to convert frame index into physical memory address
#define FRM_TO_ADDR(frame) ((FRAME0 + (frame))*NBPG)

//XXX investigate these
#define FP2FN(frm)  (((frm) - frm_tab) + FRAME0)
#define FN2ID(fn)   ((fn) - FRAME0)
#define FP2PA(frm)  ((void*)(FP2FN(frm) * NBPG))


// Macro to figure out how many Page directory entires can fit 
// in a single frame of physical memory:
//
// Note: Each page directory entry (pd_t) is 32 bits (4 bytes),
//       while a frame of memory is NBPG bytes. 
//
//       Therefore 1 frame can hold NBPG/4 entries. For a NBPG of
//       4096 this is:
//
//           4096/4 = 1024 total entries in page directory
//
//       => 1 frame of mem can store 1024 page directory entries
//
#define NENTRIES (NBPG/4)


// How many backing stores can we use? We get 
// 8 mappings (0-7). 
#define MAX_ID          7

// The backing store base address
#define BACKING_STORE_BASE  0x00800000

// The size of each backing store
#define BACKING_STORE_UNIT_SIZE 0x00100000

// Macro to convert backing store # to base address for that store
#define BSID_TO_ADDR(id) (BACKING_STORE_BASE + (id)*BACKING_STORE_UNIT_SIZE)

// Macros to determine whether backing store
// is mapped or not.
#define BSM_UNMAPPED 0
#define BSM_MAPPED   1

// Macros to determine if a memory frame is 
// mapped or not.
#define FRM_UNMAPPED 0
#define FRM_MAPPED   1

// Macros to determine the frame type. The types are:
//     FR_PAGE - contains actual logical memory
//     FR_TBL  - contains a page table
//     FR_DIR  - contains a page table directory
#define FRM_PAGE 0
#define FRM_TBL  1
#define FRM_DIR  2

// Macros used to determine the memory eviction policy
#define FIFO   3
#define AGING  4



// Backing store id type: 
//     For practical reasons each student is limited to 8 mappings
//     at a time. You will use the IDs zero through seven to identify 
//     them.
typedef unsigned int bsd_t;



// Structure for a page directory entry (PDE)
typedef struct {
    unsigned int pd_pres  : 1;        /* page table present?      */
    unsigned int pd_write : 1;        /* page is writable?        */
    unsigned int pd_user  : 1;        /* is use level protection? */
    unsigned int pd_pwt   : 1;        /* write through cachine for pt?*/
    unsigned int pd_pcd   : 1;        /* cache disable for this pt?   */
    unsigned int pd_acc   : 1;        /* page table was accessed? */
    unsigned int pd_mbz   : 1;        /* must be zero         */
    unsigned int pd_fmb   : 1;        /* four MB pages?       */
    unsigned int pd_global: 1;        /* global (ignored)     */
    unsigned int pd_avail : 3;        /* for programmer's use     */
    unsigned int pd_base  : 20;       /* location of page table?  */
} pd_t;




// Structure for a page table entry (PTE)
//     - Divide logical memory into blocks, called pages, of same 
//       size as physical memory frames
typedef struct {
    unsigned int pt_pres  : 1;        /* page is present?     */
    unsigned int pt_write : 1;        /* page is writable?        */
    unsigned int pt_user  : 1;        /* is user level protection? */
    unsigned int pt_pwt   : 1;        /* write through for this page? */
    unsigned int pt_pcd   : 1;        /* cache disable for this page? */
    unsigned int pt_acc   : 1;        /* page was accessed?       */
    unsigned int pt_dirty : 1;        /* page was written?        */
    unsigned int pt_mbz   : 1;        /* must be zero         */
    unsigned int pt_global: 1;        /* should be zero in 586    */
    unsigned int pt_avail : 3;        /* for programmer's use     */
    unsigned int pt_base  : 20;       /* location of page?        */
} pt_t;



// Structure representing a virtual address
typedef struct {
    unsigned int pg_offset : 12;      /* page offset          */
    unsigned int pt_offset : 10;      /* page table offset        */
    unsigned int pd_offset : 10;      /* page directory offset    */
} virt_addr_t;



// XXX not so sure about this crappy code
// Structure representing a backing store
typedef struct {
    int status;
    int as_heap;        /* is this bs used by heap?, if so can't be shared*/
    int npages;         /* number of pages in the store */
    bs_map_t *owners;   /* where it is mapped*/
    frame_t *frm;       /* the list of frames that maps this bs*/
} bs_t;



// Structure representing a backing store mapping
typedef struct {
    int bs_status;            /* MAPPED or UNMAPPED       */
    int bs_pid;               /* process id using this slot   */
    int bs_vpno;              /* starting virtual page number */
    int bs_npages;            /* number of pages in the store */
    int bs_sem;               /* semaphore mechanism ?    */
} bs_map_t;






// XXX evaluate this crap
frame_t *alloc_frame(int use); 

typedef struct _frame_t {
    int status; /* FRM_FREE, FRM_PGD, FRM_PGT, FRM_BS*/

    int refcnt; // If the frame is a FRM_PGT, refcnt is the number of mappings install 
                // in this PGT. release it when refcnt is zero. When the frame is
                // a FRM_BS, how many times this frame is mapped by processes. If refcnt
                // is zero, time to release the page*/

    /*Data used only if FRM_BS. The backstore pages this frame is mapping
     to and the list of all the frames for this backstore*/
    bsd_t bs;
    int bs_page;
    struct _frame_t *bs_next;



    struct _frame_t * fifo;  // List of frames for this back store XXX should this be here?


    int age; // when the page is loaded, in ticks
             // Used for page replacement policy AGING 

} frame_t;




// Structure representing a physical memory frame
//      - Divide physical memory into ﬁxed-sized blocks 
//        called frames 
typedef struct {
    int fr_status;            /* MAPPED or UNMAPPED       */
    int fr_pid;               /* process id using this frame  */
    int fr_vpno;              /* corresponding virtual page no*/
    int fr_refcnt;            /* reference count      */
    int fr_type;              /* FR_DIR, FR_TBL, FR_PAGE  */
    int fr_dirty;
    void *cookie;             /* private data structure   */
    unsigned long int fr_loadtime;    /* when the page is loaded  */
} fr_map_t;



// Table with entries representing backing store mappings
//     - See bsm.c
extern bs_map_t bsm_tab[];

// Table with entries reprensenting frame
extern fr_map_t frm_tab[];

// The first four page tables map the first 4096 pages
// (real physical memory). The information in these page tables
// is the same for all processes. We will use the following variable
// gpt (global page tables) array to keep up with these page tables.
extern pt_t gpt[];



/* Prototypes for required memory API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xunmap(int);
SYSCALL vcreate(int *, int, int, int, char *, int, long, ...);
WORD*   vgetmem(int);
SYSCALL vfreemem(bsd_t, int);
SYSCALL srpolicy(int);
SYSCALL grpolicy();


/* given calls for dealing with backing store */
SYSCALL get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);

