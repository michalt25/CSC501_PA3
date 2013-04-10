// bs.h

// There are 8 backing stores
#define NBS 8

// If counting starts at 0 what is the MAX_ID of backing stores?
#define MAX_ID (NBS-1)

// The backing store base address
#define BS_BASE  0x00800000

// The size of each backing store
#define BS_UNIT_SIZE 0x00100000

// The max # of pages that can be requested from a backing store
#define MAX_BS_PAGES (BS_UNIT_SIZE / NBPG)

// Macro to convert backing store # to base address for that store
#define BSID2PA(bsid) (BS_BASE + (bsid)*BS_UNIT_SIZE)

// Macro to determine if a given bsid is valid
#define IS_VALID_BSID(bsid) ((bsid < NBS) && (bsid >= 0))

// Macros to determine whether backing store
// is mapped or not.
#define BS_FREE 0
#define BS_USED 1

// Backing store id type: 
//     For practical reasons each student is limited to 8 mappings
//     at a time. You will use the IDs zero through seven to identify 
//     them.
typedef unsigned int bsd_t;


// Structure representing a backing store. 
typedef struct {
    bsd_t bsid;         // Which BS this is
    int status;         // BS_FREE or BS_USED 
    int isheap;         // is this bs used by heap?, if so can't be shared
    int npages;         // number of pages in the store
    bs_map_t * maps;    // where it is mapped
    frame_t  * frames;  // the list of frames that maps this bs
} bs_t;


// Structure representing a backing store mapping. Each backing store
// may be mapped into several address space. bs_map_t is used both by 
// bs_t and processes
typedef struct _bs_map_t {
    bsd_t bsid;             // Which BS is mapped here
    int pid;                // process id using this bs
    int vpno;               // starting virtual page number
    int npages;             // number of pages in the store
    struct _bs_map_t * next; 
} bs_map_t;


// Table with entries representing backing stores
//     - See bs.c
extern bs_t bs_tab[];


int init_bstab();

int alloc_bs(bsd_t bsid, int npages);

bs_t * get_free_bs(int npages);

void free_bs(bsd_t bsid);



