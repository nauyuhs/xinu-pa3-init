/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

//fr_map_t frm_map[NFRAMES];
frame_t frm_tab[NFRAMES];
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  /*kprintf("To be implemented!\n");
  return OK;*/
  int i = 0;
  for(i = 0; i < NFRAMES; i++) 
  {
	  free_frm(i);
  }
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	int i = 0;
	for (i = 0; i < NFRAMES; i++) {
		if(frm_tab[i].status == FRM_FREE){
			*avail = i;
			return OK;
		}
	}
  return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

//	kprintf("freeing frame %d\n", i);
	frm_tab[i].status = FRM_FREE;
	frm_tab[i].refcnt = 0;
	frm_tab[i].bs = -1;
	frm_tab[i].bs_page = -1;
	frm_tab[i].bs_next = NULL;
	frm_tab[i].fifo = NULL;
	frm_tab[i].age = 0;
	frm_tab[i].frm_num = FRAME0 + i;
	return OK;
}

frame_t *get_free_frame(){
	int avail = 0;
	get_frm(&avail);
	return &frm_tab[avail];
}

frame_t *get_frm_from_frm_num(int frm_num){
	int i = 0;
	for (i = 0; i < NFRAMES; i++) {
		if (frm_tab[i].frm_num == frm_num) {
			return &frm_tab[i];
		}
	}
	return 0;
}



