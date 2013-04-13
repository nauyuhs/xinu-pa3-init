/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

#define NUM_PAGES_TEST 20

//fr_map_t frm_map[NFRAMES];
frame_t frm_tab[NFRAMES];
frame_t *free_frm_list;
occupied_frm_list unfree_frm_list;

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
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
		frm_tab[i].frm_num = FRAME0 + i;
		add_to_free_frm_list(&frm_tab[i]);
  }
  unfree_frm_list.head = unfree_frm_list.tail = NULL;
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
SYSCALL free_frm(frame_t *frm)
{
//	kprintf("request to free frame %d of type %d\n", frm->frm_num, frm->fr_type);
	if(frm->fr_type == FR_PAGE){
		kprintf("remove frame %d from bs %d\n", frm->frm_num, frm->bs);
		// remove entry from bs
		bs_tab[frm->bs].pg_to_frm_map[frm->bs_page] = -1;
		// write to bs
		write_bs((char *)(frm->frm_num * NBPG), frm->bs, frm->bs_page);
		// remove entry from pg tbl
		remove_pg_tbl_entries(proctab[frm->fr_pid].pd, frm->fr_vpno, 1);
		// remove from proc list
		remove_frm_from_proc_list(frm);
	}
	remove_from_ocuupied_frm_list(frm);
	add_to_free_frm_list(frm);
	frm->bs = -1;
	frm->bs_page = -1;
	frm->status = FRM_FREE;
	return OK;
}

frame_t *get_free_frame(){
	frame_t * frm = get_from_free_frm_list();
	if(frm == NULL){
		frm = get_evicted_pg();
	}
	kprintf("alloc_frame return frame %d\n", frm ->frm_num);
	return frm;

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



void add_to_free_frm_list(frame_t *frm){
	frm->fifo = NULL;
	if(free_frm_list == NULL)
		free_frm_list = frm;
	else{
		frame_t *tmp = free_frm_list;
		while(tmp->fifo != NULL)
			tmp = tmp->fifo;
		tmp->fifo = frm;
	}
}

frame_t *get_from_free_frm_list(){
	if(free_frm_list == NULL){
//		kprintf("no frames in free list need to evict\n");
		return NULL;
	}
	// get the first free frame
	frame_t * tmp = free_frm_list;
	free_frm_list = tmp->fifo;
	add_to_ocuupied_frm_list(tmp);
	return tmp;
}

void add_to_ocuupied_frm_list(frame_t *frm){
	frm->fifo = NULL;
	if(unfree_frm_list.head == NULL){
		unfree_frm_list.head = unfree_frm_list.tail = frm;
	}
	unfree_frm_list.tail->fifo = frm;
	unfree_frm_list.tail = frm;
}

void remove_from_ocuupied_frm_list(frame_t *frm){
	frame_t *prev = unfree_frm_list.head;
	frame_t *curr = unfree_frm_list.head;
	while(curr != NULL ){
		if(curr == frm){
			prev->fifo = curr->fifo;
			if(curr == unfree_frm_list.head){
				unfree_frm_list.head = curr->fifo;
			}
			if(curr == unfree_frm_list.tail){
				unfree_frm_list.tail = prev;
			}
			curr->fifo = NULL;
			return;
		}
		prev= curr;
		curr = curr->fifo;
	}
}

frame_t * get_evicted_pg(){
	frame_t *frm;
	if (grpolicy() == FIFO)
		frm = fifo_evict_policy();
	else
		frm = aging_evict_policy();

	free_frm(frm); // free the frame
	remove_from_free_frm_list(frm); // remove from free list
	add_to_ocuupied_frm_list(frm); // add to unfree list
	return frm;
}

frame_t *fifo_evict_policy(){
//	kprintf("fifo eviction policy called\n");
	frame_t *curr = unfree_frm_list.head;
	while(curr != NULL){
		if(curr->fr_type == FR_PAGE) return curr;
		curr = curr->fifo;
	}
	return 0;
}

frame_t *aging_evict_policy(){
//	kprintf("aging eviction policy called\n");
	frame_t *frm_with_lowest_age = NULL;
	frame_t *curr = unfree_frm_list.head;
	while(curr != NULL){
		if(curr->fr_type == FR_PAGE){
			curr->age = (curr->age >> 1);
			if(has_page_been_accessed(currpid, curr->fr_vpno)){
				curr->age |= AGE_FACTOR;
				make_page_access_zero(currpid, curr->fr_vpno);
			}
			if(frm_with_lowest_age == NULL)
				frm_with_lowest_age = curr;
			else if(frm_with_lowest_age->age > curr->age)
				frm_with_lowest_age = curr;
		}
		curr = curr->fifo;
	}
	return frm_with_lowest_age;
}

void remove_from_free_frm_list(frame_t *frm){
	if (free_frm_list == NULL) {
//		kprintf("no frames present to free\n");
		return;
	}
	frame_t *curr = free_frm_list;
	frame_t *prev = free_frm_list;
	while(curr != NULL){
		if(curr == frm){
			if(free_frm_list == curr)
				free_frm_list = curr->fifo;
			prev->fifo = curr->fifo;
			curr->fifo = NULL;
			return;
		}
		prev = curr;
		curr = curr->fifo;
	}
}


