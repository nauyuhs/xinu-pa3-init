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
	bs_map_t owner/* = (bs_map_t * )malloc(sizeof(bs_map_t))*/;
	owner.bs = source;
	owner.next = NULL;
	owner.npages = npages;
	owner.pid = pid;
	owner.vpno = vpno;
	if (tab->owners == NULL)
		tab->owners = &owner;
	else {
		bs_map_t *temp = tab->owners;
		while (temp->next != NULL)
			temp = temp->next;
		temp->next = &owner;
	}
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
	int frm_num = 0;
	if(bs_tab[id].pg_to_frm_map[pageth] != -1){
		frm_num = bs_tab[id].pg_to_frm_map[pageth];
	}
	else{
		get_frm(&frm_num);
		bs_tab[id].pg_to_frm_map[pageth] = frm_num;
		// put mapping in bs
		if(bs_tab[id].frm == NULL)
			bs_tab[id].frm  = &frm_tab[frm_num];
		else{
			frame_t *temp = bs_tab[id].frm ;
			while(temp->bs_next != NULL)
				temp  = temp->bs_next;
			temp->bs_next = &frm_tab[frm_num];
		}
		// put mapping in frame
		frm_tab[frm_num].bs = id;
		frm_tab[frm_num].bs_page = pageth;
		frm_tab[frm_num].status = FRM_BS;

		frm_map[frm_num].fr_status = FRM_MAPPED;
		frm_map[frm_num].fr_type = FRM_BS;
		frm_map[frm_num].fr_vpno = proctab[currpid].map[id].vpno + pageth;

		// now bring the page into memory
		read_bs((char *)(frm_tab[frm_num].frm_num * NBPG), id, pageth );
	}
	return &frm_tab[frm_num];
}

SYSCALL remove_owner_mapping(bsd_t source, int pid){
	bs_t *tab = &bs_tab[source];
	bs_map_t *temp = tab->owners;
	while(temp != NULL){
		if(temp->next->pid == pid){
			bs_map_t *next = temp->next;
			if(temp == tab->owners){
				tab->owners = NULL;
			}
			else
				temp->next = next->next;
			next->next = NULL;
			return OK;
		}
		temp = temp->next;
	}
	return SYSERR;

}

void free_frms_for_bs(bsd_t id){
	int i;
	for(i = 0; i < NUM_BS_PGS; i++){
		if(bs_tab[id].pg_to_frm_map[i] > -1){
			free_bs_frame(bs_tab[id].pg_to_frm_map[i]);
			// remove it from the list as well
			frame_t *temp = bs_tab[id].frm ;
			while(temp != NULL){
				if(temp->bs_next->frm_num == bs_tab[id].pg_to_frm_map[i]){
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
	write_bs(frm_tab[frm_num].frm_num * NBPG,
			frm_tab[frm_num].bs, frm_tab[frm_num].bs_page);
	frm_tab[frm_num].bs = -1;
	frm_tab[frm_num].bs_page = -1;
	frm_tab[frm_num].status = FRM_FREE;

	frm_map[frm_num].fr_status = FRM_UNMAPPED;
	frm_map[frm_num].fr_type = FRM_FREE;
	frm_map[frm_num].fr_vpno = 0;
}


