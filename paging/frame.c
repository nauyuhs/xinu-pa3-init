/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_map[NFRAMES];
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
	frm_tab[i].status = FRM_FREE;
	frm_tab[i].refcnt = 0;
	frm_tab[i].bs = -1;
	frm_tab[i].bs_page = -1;
	frm_tab[i].bs_next = NULL;
	frm_tab[i].fifo = NULL;
	frm_tab[i].age = 0;
	
	frm_map[i].fr_status = FRM_UNMAPPED;
	frm_map[i].fr_pid = -1;
	frm_map[i].fr_vpno = -1;
	frm_map[i].fr_refcnt = 0;
	frm_map[i].fr_type = FR_PAGE;
	frm_map[i].fr_dirty = -1;
	frm_map[i].cookie = NULL;
	frm_map[i].fr_loadtime = 0;

  }

}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  kprintf("To be implemented!\n");
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

  kprintf("To be implemented!\n");
  return OK;
}



