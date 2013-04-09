#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>



init_glb_pgs(int *idx_mapper){
	int i, avail;
	for (i = 0; i < NUM_GLB_PG_TBLS; i++) {
		avail = 0;
		get_frm(&avail);
		frm_tab[avail].status = FRM_PGT;
		frm_tab[avail].refcnt = MAXINT;
		*(idx_mapper + i) = frm_tab[avail].frm_num;

		frm_map[avail].fr_pid = 0;
		frm_map[avail].fr_status = FRM_MAPPED;
		frm_map[avail].fr_type = FR_TBL;
	}
	pt_t *tmp = (pt_t *)(NBPG*NFRAMES);
	for (i = 0; i < NUM_PG_TBL_ENTRIES * NUM_GLB_PG_TBLS; i++) {
		pt_t ptr;
		ptr.pt_pres = 1;
		ptr.pt_write = 1;
		ptr.pt_user = 0;
		ptr.pt_pwt = 0;
		ptr.pt_pcd = 0;
		ptr.pt_acc = 0;
		ptr.pt_dirty = 0;
		ptr.pt_mbz = 0;
		ptr.pt_global = 0;
		ptr.pt_avail = 0;
		ptr.pt_base = i;
		*tmp  = ptr;
		tmp++;
	}
}

SYSCALL init_pg_dir(int *num, int pid) {
	int avail = 0, i;
	get_frm(&avail);
	*num = avail;
	frm_tab[avail].status = FRM_PGD;
	frm_tab[avail].refcnt = MAXINT;

	frm_map[avail].fr_pid = pid;
	frm_map[avail].fr_status = FRM_MAPPED;
	frm_map[avail].fr_type = FR_DIR;

	pd_t *tmp1 = (pd_t *) (NBPG * frm_tab[avail].frm_num);
	for (i = 0; i < NUM_GLB_PG_TBLS; i++) {
		pd_t ptr1;
		ptr1.pd_pres = 1 ;
		ptr1.pd_write = 1;
		ptr1.pd_user = 0;
		ptr1.pd_pwt = 0;
		ptr1.pd_pcd = 0;
		ptr1.pd_acc = 0;
		ptr1.pd_mbz = 0;
		ptr1.pd_fmb = 0;
		ptr1.pd_global = 0;
		ptr1.pd_avail = 0;
		ptr1.pd_base = glb_pg_tbl_frm_mapping[i];
		*tmp1 = ptr1;
		tmp1++;
	}
	return OK;
}

SYSCALL free_pg_dir(frame_t *pd){
	int i;
	pd_t *ptr1 = (pd_t *)(NBPG * pd->frm_num);
	kprintf("freeing dir for proc %d\n", currpid);
	// dont free global pages
	ptr1 += NUM_GLB_PG_TBLS;
	// free the page tables
	for(i = NUM_GLB_PG_TBLS; i < NUM_PG_TBL_ENTRIES; i++){
		if(ptr1->pd_pres == 1 ){
			free_frm(ptr1->pd_base);
			ptr1++;
		}
	}
	kprintf("freed pd dir at %d \n", pd->frm_num);
	// free the dir
	free_frm(pd->frm_num);
	pd = NULL;
	return OK;
}

void uninit_pg_tbl(int frm_num){
	int i;
	pt_t *ptr = (pt_t *) (NBPG * frm_num);
	for (i = 0; i < NUM_PG_TBL_ENTRIES; i++) {
		ptr->pt_pres = 0;
		ptr->pt_write = 0;
		ptr->pt_user = 0;
		ptr->pt_pwt = 0;
		ptr->pt_pcd = 0;
		ptr->pt_acc = 0;
		ptr->pt_dirty = 0;
		ptr->pt_mbz = 0;
		ptr->pt_global = 0;
		ptr->pt_avail = 0;
		ptr->pt_base = 0;
		ptr++;
	}
}

void uninit_pg_dir(int frm_num){
	int  i;
	pd_t *ptr1 = (pd_t *) (NBPG * frm_num);
	for (i = 0; i < NUM_PG_TBL_ENTRIES; i++) {
			ptr1->pd_pres = 0 ;
			ptr1->pd_write = 0;
			ptr1->pd_user = 0;
			ptr1->pd_pwt = 0;
			ptr1->pd_pcd = 0;
			ptr1->pd_acc = 0;
			ptr1->pd_mbz = 0;
			ptr1->pd_fmb = 0;
			ptr1->pd_global = 0;
			ptr1->pd_avail = 0;
			ptr1->pd_base = 0;
			ptr1++;
		}
}

unsigned long add_pg_dir_entry_for_pg_fault(int pid, unsigned int pg_dir_offset,
		unsigned int pg_tbl_offset, frame_t * frm ){
	unsigned long pg_tbl_frm;
	int avail = 0, i;
	struct pentry *pptr = &proctab[pid];
	frame_t *pg_dir = pptr->pd;
	pd_t *tmp1 = (pd_t *) ((NBPG * pg_dir->frm_num) + pg_dir_offset * sizeof(pd_t));
	if (tmp1->pd_pres == 1) {

		pg_tbl_frm = tmp1->pd_base;
		pt_t *tmp = (pt_t *) ((pg_tbl_frm * NBPG) + pg_tbl_offset * sizeof(pt_t));
		pt_t ptr;
		ptr.pt_pres = 1;
		ptr.pt_write = 1;
		ptr.pt_user = 0;
		ptr.pt_pwt = 0;
		ptr.pt_pcd = 0;
		ptr.pt_acc = 0;
		ptr.pt_dirty = 0;
		ptr.pt_mbz = 0;
		ptr.pt_global = 0;
		ptr.pt_avail = 0;
		ptr.pt_base = frm->frm_num ;
		*tmp = ptr;
	}
	else{
		// create pd entry as it is absent
		get_frm(&avail);
		pd_t ptr1;
		ptr1.pd_pres = 1;
		ptr1.pd_write = 1;
		ptr1.pd_user = 0;
		ptr1.pd_pwt = 0;
		ptr1.pd_pcd = 0;
		ptr1.pd_acc = 0;
		ptr1.pd_mbz = 0;
		ptr1.pd_fmb = 0;
		ptr1.pd_global = 0;
		ptr1.pd_avail = 0;
		ptr1.pd_base = frm_tab[avail].frm_num ;
		kprintf("writing page tbl frm %d at idx %d\n", ptr1.pd_base, pg_dir_offset);
		*tmp1 = ptr1;

		frm_tab[avail].status = FRM_PGT;
		frm_tab[avail].refcnt = MAXINT;

		frm_map[avail].fr_pid = pid;
		frm_map[avail].fr_status = FRM_MAPPED;
		frm_map[avail].fr_type = FR_TBL;

		pg_tbl_frm = frm_tab[avail].frm_num ;
	}
    return pptr->pdbr;
}
