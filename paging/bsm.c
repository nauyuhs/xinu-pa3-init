/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bs_map[NBS];
bs_t bs_tab[NBS];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	int i = 0, j = 0;
	for(i = 0;i < NBS; i++)
	{
		bs_tab[i].status = BSM_UNMAPPED;
		bs_tab[i].as_heap = 0;
		bs_tab[i].npages = -1;
		bs_tab[i].owners = NULL;
		bs_tab[i].frm = NULL;
		for(j = 0 ; j < NUM_BS_PGS; j++)
			bs_tab[i].pg_to_frm_map[j] = -1; /* init with no mapping  */
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail) {
	int i;
	for (i = 0; i < NBS; i++) {
		if (bs_tab[i].status == BSM_UNMAPPED) {
			return *avail = i;
			return OK;
		}
	}
	return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	bs_tab[i].status = BSM_UNMAPPED;
	bs_tab[i].npages = 0;
	bs_tab[i].as_heap = 0;
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	int  i;
	struct pentry *pptr = &proctab[pid];
	for(i = 0 ; i< NBS; i++){
		bs_map_t *map = &(pptr->map[i]);
		if(map->status == BSM_MAPPED && (((map->vpno + map->npages) * NBPG) >= vaddr) &&
				(map->vpno  * NBPG <= vaddr)){
			*store = i;
			*pageth = find_page(map->vpno, map->npages, vaddr);
			return OK;
		}
	}
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages) {

	if (bs_tab[source].status == BSM_UNMAPPED) {
		bs_tab[source].status = BSM_MAPPED;
		bs_tab[source].npages = npages;
	}

	struct pentry *pptr = &proctab[pid];
	bs_map_t *map = &(pptr->map[source]);
	map->bs = source;
	map->pid = pid;
	map->npages = npages;
	map->vpno = vpno;
	map->status = BSM_MAPPED;

	bs_t *tab = &bs_tab[source];
	bs_map_t *owner = (bs_map_t *)getmem(sizeof(bs_map_t));
	owner->bs = source;
	owner->npages = npages;
	owner->pid = pid;
	owner->vpno = vpno;
	owner->next = tab->owners;
	tab->owners = owner;
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	int i, store, pageth;
	struct pentry *pptr = &proctab[pid];
	bsm_lookup(pid, vpno*NBPG, &store, &pageth);
	bs_map_t *map = &(pptr->map[store]);
	frame_t *frms= map->frm;
	while(frms != NULL){
		frame_t *temp = frms;
		frms = frms->bs_next;
//		free_frm(temp);
		free_shared_frm(temp, pid);
	}
	remove_owner_mapping(map->bs, pid);
	map->frm =  NULL;
	map->bs = -1;
	map->npages = 0;
	map->status = BSM_UNMAPPED;
	map->vpno = 0;
	if(bs_tab[store].owners == NULL)
		free_bsm(store);
	kprintf("delete pid %d's mapping of bs%d@%d\n", pid, store, vpno);
	return OK;
}

void remove_frm_from_proc_list(frame_t *frm){
	struct pentry *pptr = &proctab[frm->fr_pid];
	bs_map_t *map = &(pptr->map[frm->bs]);
	frame_t *curr= map->frm;
	frame_t *prev= map->frm;
	while(curr != NULL){
		if(curr == frm){
			if(curr == map->frm)
				map->frm = curr->bs_next;
			prev->bs_next = curr->bs_next;
			curr->bs_next = NULL;
			break;
		}
		prev = curr;
		curr = curr->bs_next;
	}
}

int find_page(int start_vpage, int npages, int vaddr){
	int  i;
	for(i = start_vpage; i < (start_vpage + npages); i++)
		if(vaddr >= i*NBPG && vaddr <= (i*NBPG + (NBPG - 1)))
			 return i - start_vpage;
	return (int)0;
}

frame_t *bs_get_frame(bsd_t id, int pageth) {
	frame_t *bs_frm;
	if (bs_tab[id].pg_to_frm_map[pageth] != -1) {
		bs_frm = get_frm_from_frm_num(bs_tab[id].pg_to_frm_map[pageth]);
	} else {
		bs_frm = get_free_frame();
		bs_tab[id].pg_to_frm_map[pageth] = bs_frm->frm_num;
		// put mapping in frame
		bs_frm->bs = id;
		bs_frm->bs_page = pageth;
		bs_frm->status = FRM_BS;
		bs_frm->fr_type = FR_PAGE;
		// now bring the page into memory
		read_bs((char *) (bs_frm->frm_num * NBPG), id, pageth);
	}
	kprintf("map bs%d/page: %d to frame %d\n", id, pageth, bs_frm->frm_num);
	return bs_frm;
}

SYSCALL remove_owner_mapping(bsd_t source, int pid){
	bs_t *tab = &bs_tab[source];
	bs_map_t *prev = NULL;
	bs_map_t *temp = tab->owners;
	while(temp != NULL){
		if(temp->pid == pid){
			if(temp == tab->owners){
				tab->owners = NULL;
			}
			else{
				prev->next = temp->next;
				temp->next = NULL;
			}
			freemem((struct mblock *)temp, sizeof(bs_map_t));
			return OK;
		}
		prev = temp;
		temp = temp->next;
	}
	return SYSERR;
}

void free_frms_for_bs(bsd_t id){
	int i, frm_num;
	for(i = 0; i < NUM_BS_PGS; i++){
		frm_num = bs_tab[id].pg_to_frm_map[i];
		if(frm_num > -1){
			free_bs_frame(frm_num);
			bs_tab[id].pg_to_frm_map[i] = -1;
		}
	}

}

void free_bs_frame(int frm_num){
	frame_t *frm = get_frm_from_frm_num(frm_num);
	write_bs((char *)(frm->frm_num * NBPG),
			frm->bs, frm->bs_page);
	frm->bs = -1;
	frm->bs_page = -1;
	frm->status = FRM_FREE;
}

void add_mapping_to_proc_frm_list(frame_t *frm, bsd_t id, int pid){
	struct pentry *pptr = &proctab[pid];
	bs_map_t *map = &(pptr->map[id]);
	if(map->frm == NULL)
		map->frm = frm;
	else{
		frame_t *tmp = map->frm;
		while(tmp->bs_next != NULL)
			tmp = tmp->bs_next;
		tmp->bs_next = frm;
	}
	frm->fr_vpno = map->vpno + frm->bs_page;
	frm->fr_pid = pid;
	add_proc_for_bs_frame(frm, map->vpno + frm->bs_page, pid);
}
