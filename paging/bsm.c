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

		bs_map[i].next = NULL;
		bs_map[i].bs = BSM_UNMAPPED;
		bs_map[i].pid = -1;
		bs_map[i].vpno = -1;
		bs_map[i].npages = 0;
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	kprintf("free request for bs = %d\n", i);
	bs_tab[i].status = BSM_UNMAPPED;
	bs_tab[i].npages = 0;
	kprintf("freeing the mapped frms for the bs\n");
	free_frms_for_bs(i);
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
		if(map->status == BSM_MAPPED && ((map->vpno + map->npages) * NBPG) > vaddr ){
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

	kprintf("mapping called for bs = %d\n", source);
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
	kprintf("calling unmap for pid = %d and vpno = %d\n", pid, vpno);
	int i;
	struct pentry *pptr = &proctab[pid];

	for(i = 0; i < NBS; i++){
		bs_map_t *map = &(pptr->map[i]);
		if(map->status == BSM_MAPPED && map->vpno == vpno){
			remove_pg_tbl_entries(pptr->pd, map->vpno , map->npages);
			map->status = BSM_UNMAPPED;
			map->vpno  = 0;
			map->npages = 0;
			remove_owner_mapping(map->bs, pid);
			if(bs_tab[map->bs].owners == NULL){
				free_bsm(map->bs);
			}
		}
	}
	return OK;
}


int find_page(int start_vpage, int npages, int vaddr){
	int  i;
	for(i = start_vpage; i < (start_vpage + npages); i++){
		if(vaddr >= i*NBPG && vaddr <= (i*NBPG + (NBPG - 1))){
			 i -= start_vpage;
			 return i;
		}
	}
	return (int)0;
}

frame_t *bs_get_frame(bsd_t id, int pageth){
	if(bs_tab[id].pg_to_frm_map[pageth] != -1){
		return get_frm_from_frm_num(bs_tab[id].pg_to_frm_map[pageth]);
	}
	else{
		frame_t *bs_frm = get_free_frame();
		bs_tab[id].pg_to_frm_map[pageth] = bs_frm->frm_num;
		// put mapping in bs
		if(bs_tab[id].frm == NULL)
			bs_tab[id].frm  = bs_frm;
		else{
			frame_t *temp = bs_tab[id].frm ;
			while(temp->bs_next != NULL)
				temp  = temp->bs_next;
			temp->bs_next = bs_frm;
		}
		// put mapping in frame
		bs_frm->bs = id;
		bs_frm->bs_page = pageth;
		bs_frm->status = FRM_BS;
		bs_frm->fr_type = FR_PAGE;
		// now bring the page into memory
		read_bs((char *)(bs_frm->frm_num * NBPG), id, pageth );
		return bs_frm;
	}
}

SYSCALL remove_owner_mapping(bsd_t source, int pid){
	kprintf("source = %d and pid = %d\n", source, pid);
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
			kprintf("owner removed\n");
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
			// remove it from the list as well
			frame_t *temp = bs_tab[id].frm ;
			while(temp != NULL){
				if(temp->bs_next->frm_num == frm_num){
					frame_t *nxt = temp->bs_next;
					if(temp == bs_tab[id].frm ) bs_tab[id].frm = NULL;
					else
						temp->bs_next = nxt->bs_next;
					nxt->bs_next = NULL;
					break;
				}
				temp  = temp->bs_next;
			}
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




