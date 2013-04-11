/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

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
	kprintf("request to free frame %d of type %d\n", frm->frm_num, frm->fr_type);
	if(frm->fr_type == FR_PAGE){
		write_bs((char *)(frm->frm_num * NBPG), frm->bs, frm->bs_page);
	}
	remove_from_ocuupied_frm_list(frm);
	add_to_free_frm_list(frm);
	frm->bs = -1;
	frm->bs_page = -1;
	frm->status = FRM_FREE;
	return OK;
}

frame_t *get_free_frame(){
	return get_from_free_frm_list();
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
		kprintf("no frames in free list need to evict\n");
		return NULL;
	}
	// get the first free frame
	frame_t * tmp = free_frm_list;
	free_frm_list = tmp->fifo;
	add_to_ocuupied_frm_list(tmp);
	kprintf("returning frm %d from free list \n", tmp->frm_num);
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
			kprintf("rmoving frm occupied list frm = %d\n", curr->frm_num);
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



