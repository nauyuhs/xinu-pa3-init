/* paging.h */
#ifndef _PAGING_H_
#define _PAGING_H_

typedef unsigned int	 bsd_t;
//#define FP2FN(frm)  (((frm) - frm_map) + FRAME0)
#define FN2ID(fn)   ((fn) - FRAME0)
#define FP2PA(frm)  ((void*)(FP2FN(frm) * NBPG))
#define VALID_MEM_FRM(frm) (frm >= 1024 && frm <= 2047)
#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/

#define NFRAMES 	25	/* number of frames		*/

#define NUM_PG_TBL_ENTRIES 1024
#define BSM_UNMAPPED	0
#define BSM_MAPPED	1

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define FRM_FREE 0
#define FRM_PGD 1
#define FRM_PGT 2
#define FRM_BS 3

#define FIFO		3
#define AGING		4

#define MAX_ID          7              /* You get 8 mappings, 0 - 8 */

#define BACKING_STORE_BASE	0x00800000

#define BACKING_STORE_UNIT_SIZE 0x00100000

#define NBS 8
/* Structure for a page directory entry */

#define NUM_GLB_PG_TBLS 4

#define NUM_BS_PGS 256

#define AGE_FACTOR 0x80

#define REMOVE_ALL -3

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset ;		/* page offset			*/
  unsigned int pt_offset ;		/* page table offset		*/
  unsigned int pd_offset ;		/* page directory offset	*/
} virt_addr_t;

typedef struct _proc_frm_t{
	struct _proc_frm_t *next ;
	int pid;
	unsigned int vpno;
}proc_frm_t;

typedef struct _frame_t {
	int status; /* FRM_FREE, FRM_PGD, FRM_PGT, FRM_BS*/

	/*If the frame is a FRM_PGT, refcnt is the number of mappings install
	 in this PGT. release it when refcnt is zero. When the frame is
	 a FRM_BS, how many times this frame is mapped by processes. If refcnt
	 is zero, time to release the page*/
	int refcnt;

	/*Data used only if FRM_BS. The backstore pages this frame is mapping
	 to and the list of all the frames for this backstore*/
	bsd_t bs;
	int bs_page;
	struct _frame_t *bs_next;

	struct _frame_t *fifo; /* when the page is loaded, in ticks*/
	int age; /* Used for page replacement policy AGING */
	int frm_num;
	int fr_pid;				/* process id using this frame  */
	int fr_vpno;				/* corresponding virtual page no*/
	int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
	int fr_dirty;
	void *cookie;				/* private data structure	*/
	unsigned long int fr_loadtime;	/* when the page is loaded 	*/
	proc_frm_t *procs;
} frame_t; //kernel

extern frame_t *free_frm_list;

typedef struct{
	frame_t *head;
	frame_t *tail;
}occupied_frm_list;

extern occupied_frm_list unfree_frm_list;



//typedef struct{
 // int bs_status;			/* MAPPED or UNMAPPED		*/
 // int bs_pid;				/* process id using this slot   */
  //int bs_vpno;				/* starting virtual page number */
  //int bs_npages;			/* number of pages in the store */
  //int bs_sem;				/* semaphore mechanism ?	*/
//} bs_map_t;

/*Each back store may be mapped into several address space.
 bs_map_t is used both by bs_t and process*/
typedef struct _bs_map_t {
	struct _bs_map_t *next;
	bsd_t bs; /* which bs is mapped*/
	int pid; /* process mapped this backstore*/
	int vpno; /* first virtual page the bs mapped to*/
	int npages;
	int status;
	frame_t *frm; /* the list of frames that maps this bs*/
} bs_map_t; //kernel

typedef struct {
	int pg_to_frm_map[NUM_BS_PGS];
	int status;
	int as_heap; /* is this bs used by heap?*/
	int npages; /* number of pages in the store */
	bs_map_t *owners; /* where it is mapped*/
	frame_t *frm; /* the list of frames that maps this bs*/
} bs_t; //kernel






//typedef struct{
//  int fr_status;			/* MAPPED or UNMAPPED		*/
//  int fr_pid;				/* process id using this frame  */
//  int fr_vpno;				/* corresponding virtual page no*/
//  int fr_refcnt;			/* reference count number of things pointing to your page*/
//  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
//  int fr_dirty;
//  void *cookie;				/* private data structure	*/
//  unsigned long int fr_loadtime;	/* when the page is loaded 	*/
//}fr_map_t;

typedef struct _mem_list{
	struct _mem_list *next;
	char *mem;
	int memlen;
}mem_list;



extern bs_map_t bs_map[NBS];
//extern fr_map_t frm_map[NFRAMES];
extern bs_t bs_tab[NBS];
extern frame_t frm_tab[NFRAMES];
extern int glb_pg_tbl_frm_mapping[];

/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xunmap(int);

/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);
void init_glb_pgs(int *idx_mapper);
SYSCALL init_pg_dir(frame_t *frm, int pid);
SYSCALL free_pg_dir(frame_t *pd);
SYSCALL free_frm(frame_t *frm);
int find_page(int start_vpage, int npages, int vaddr);
void uninit_pg_tbl(int frm_num);
void uninit_pg_dir(int frm_num);
frame_t *bs_get_frame(bsd_t id, int pageth);
unsigned long add_entry_for_pg_fault(int pid, unsigned long vaddr, frame_t * frm );
SYSCALL remove_owner_mapping(bsd_t id, int pid);
void free_frms_for_bs(bsd_t id);
void free_bs_frame(int frm_num);

frame_t *get_free_frame();

void make_pg_tbl_entry(frame_t *frm, int idx, int base);

void remove_pg_tbl_entry(frame_t *frm, int idx);

frame_t * create_pg_tbl(int pid);

void make_pg_dir_entry(frame_t *frm, int idx, int base);

void remove_pg_dir_entry(frame_t *frm, int idx);

frame_t *get_frm_from_frm_num(int frm_num);

void remove_pg_tbl_entries(frame_t *pg_dir, int vaddr, int num_pgs);

void get_virt_addr(virt_addr_t *vaddr_t, unsigned long vaddr);

void init_frm_lists();

void add_to_free_frm_list(frame_t *frm);
frame_t *get_from_free_frm_list();
void add_to_ocuupied_frm_list(frame_t *frm);

void add_mapping_to_proc_frm_list(frame_t *frm, bsd_t id, int pid);

void remove_from_ocuupied_frm_list(frame_t *frm);

bs_t *get_free_bs();

frame_t *get_evicted_pg();
frame_t *fifo_evict_policy();

void remove_frm_from_proc_list(frame_t *frm);

void remove_from_free_frm_list(frame_t *frm);

frame_t *aging_evict_policy();

int has_page_been_accessed(int pid, int vpno);


void write_pg_tbl_entry(frame_t *frm, int idx, unsigned int pt_pres, unsigned int pt_write, unsigned int base);
void write_pg_dir_entry(frame_t *frm, int idx, unsigned int pd_pres, unsigned int pd_write, unsigned int base);
unsigned long get_vaddr_from_vpno(int vpno);
void get_offsets_from_vaddr(unsigned long vaddr, unsigned int *pd_offset, unsigned int *pt_offset);
pd_t *get_pg_dir_entry(frame_t *pg_dir, int offset);

void  make_page_access_zero(int pid, int vpno);

void add_proc_for_bs_frame(frame_t *frm, unsigned int vpno, int pid);

proc_frm_t *remove_proc_mapping_from_bs_frame(frame_t *frm, int pid);

SYSCALL free_shared_frm(frame_t *frm, int  pid);

void unmap_pg_tbl_entry(frame_t *frm,  int pid);

int is_bs_frm_unmapped(frame_t *frm);

proc_frm_t *remove_first__from_proc_mapping_from_bs_frame(frame_t *frm);
proc_frm_t *get_first__from_proc_mapping_from_bs_frame(frame_t *frm);

void print_free_mem_status();

int is_free_frm_list_empty();
/*creating common 4 page tables and 1 page directory
pt_t shared_page_table[4][1024];
pd_t shared_page_directory[4];*/

#endif
