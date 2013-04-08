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
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
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


