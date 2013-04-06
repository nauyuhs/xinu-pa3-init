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
	pt_t *ptr = (pt_t *)(NBPG*NFRAMES);
	for (i = 0; i < NUM_PG_TBL_ENTRIES * NUM_GLB_PG_TBLS; i++) {
		ptr->pt_pres = 1;
		ptr->pt_write = 1;
		ptr->pt_user = 0;
		ptr->pt_pwt = 0;
		ptr->pt_pcd = 0;
		ptr->pt_acc = 0;
		ptr->pt_dirty = 0;
		ptr->pt_mbz = 0;
		ptr->pt_global = 0;
		ptr->pt_avail = 0;
		ptr->pt_base = i;
		ptr++;
	}
}

SYSCALL init_pg_dir(int *num) {
	int avail = 0, i;
	get_frm(&avail);
	*num = avail;
	frm_tab[avail].status = FRM_PGD;
	frm_tab[avail].refcnt = MAXINT;

	frm_map[avail].fr_pid = 0;
	frm_map[avail].fr_status = FRM_MAPPED;
	frm_map[avail].fr_type = FR_DIR;

	pd_t *ptr1 = (pd_t *) (NBPG * frm_tab[avail].frm_num);
	for (i = 0; i < NUM_GLB_PG_TBLS; i++) {
		ptr1->pd_pres = 1 ;
		ptr1->pd_write = 1;
		ptr1->pd_user = 0;
		ptr1->pd_pwt = 0;
		ptr1->pd_pcd = 0;
		ptr1->pd_acc = 0;
		ptr1->pd_mbz = 0;
		ptr1->pd_fmb = 0;
		ptr1->pd_global = 0;
		ptr1->pd_avail = 0;
		ptr1->pd_base = glb_pg_tbl_frm_mapping[i];
		ptr1++;
	}
	return OK;
}

SYSCALL free_pg_dir(frame_t *pd){
	int i;
	pd_t *ptr1 = (pd_t *)(NBPG * pd->frm_num);
	// dont free global pages
	ptr1 += NUM_GLB_PG_TBLS;
	// free the page tables
	for(i = NUM_GLB_PG_TBLS; i < NUM_PG_TBL_ENTRIES; i++){
		if(ptr1->pd_pres == 1 ){
			uninit_pg_tbl(ptr1->pd_base);
			free_frm(ptr1->pd_base);
			ptr1++;
		}
	}
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
