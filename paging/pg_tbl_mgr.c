#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>



init_glb_pgs(int *idx_mapper){
	int i, avail, j;
	for (i = 0; i < NUM_GLB_PG_TBLS; i++) {
		frame_t *pg_tbl = create_pg_tbl(NULLPROC);
		*(idx_mapper + i) = pg_tbl->frm_num;
		for(j = 0; j< NUM_PG_TBL_ENTRIES ; j++)
			make_pg_tbl_entry(pg_tbl, j, i*NUM_PG_TBL_ENTRIES + j);
	}
}

frame_t * create_pg_tbl(int pid){
	frame_t *pg_tbl = get_free_frame();
	pg_tbl->status = FRM_PGT;
	pg_tbl->fr_type = FRM_PGT;
	pg_tbl->fr_pid = pid;
	return pg_tbl;
}


void make_pg_tbl_entry(frame_t *frm, int idx, int base){
	pt_t *tmp = (pt_t *) (NBPG * frm->frm_num );
	tmp += idx;
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
	ptr.pt_base = base;
	*tmp = ptr;
}

void remove_pg_tbl_entry(frame_t *frm, int idx){
	pt_t *tmp = (pt_t *) (NBPG * frm->frm_num);
	tmp += idx;
	pt_t ptr;
	ptr.pt_pres = 0;
	ptr.pt_write = 0;
	ptr.pt_user = 0;
	ptr.pt_pwt = 0;
	ptr.pt_pcd = 0;
	ptr.pt_acc = 0;
	ptr.pt_dirty = 0;
	ptr.pt_mbz = 0;
	ptr.pt_global = 0;
	ptr.pt_avail = 0;
	ptr.pt_base = 0;
	*tmp = ptr;
}

void make_pg_dir_entry(frame_t *frm, int idx, int base){
	pd_t *tmp1 = (pd_t *) (NBPG * frm->frm_num);
	tmp1 += idx;
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
	ptr1.pd_base = base;
	*tmp1 = ptr1;
}

void remove_pg_dir_entry(frame_t *frm, int idx){
	pd_t *tmp1 = (pd_t *) (NBPG * frm->frm_num);
	tmp1 += idx;
	pd_t ptr1;
	ptr1.pd_pres = 0;
	ptr1.pd_write = 0;
	ptr1.pd_user = 0;
	ptr1.pd_pwt = 0;
	ptr1.pd_pcd = 0;
	ptr1.pd_acc = 0;
	ptr1.pd_mbz = 0;
	ptr1.pd_fmb = 0;
	ptr1.pd_global = 0;
	ptr1.pd_avail = 0;
	ptr1.pd_base = 0;
	*tmp1 = ptr1;
}


SYSCALL init_pg_dir(frame_t *frm, int pid) {
	int avail = 0, i;
	frm->fr_pid = pid;
	frm->fr_type = FR_DIR;
	frm->status = FRM_PGD;

	for (i = 0; i < NUM_GLB_PG_TBLS; i++) {
		make_pg_dir_entry(frm, i, glb_pg_tbl_frm_mapping[i]);
	}
	return OK;
}

SYSCALL free_pg_dir(frame_t *pd){
	int i;
	pd_t *ptr1 = (pd_t *)(NBPG * pd->frm_num);
	// dont free global pages
	ptr1 += NUM_GLB_PG_TBLS;
	// free the page tables
	for(i = 0; i < NUM_PG_TBL_ENTRIES-NUM_GLB_PG_TBLS; i++){
		if(ptr1->pd_pres == 1 ){
			free_frm(get_frm_from_frm_num(ptr1->pd_base));
		}
		ptr1++;
	}
	kprintf("freed pg dir at %d \n", pd->frm_num);
	// free the dir
	free_frm(get_frm_from_frm_num(pd->frm_num));
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
		frame_t *pg_tbl = get_frm_from_frm_num(tmp1->pd_base);
		make_pg_tbl_entry(pg_tbl, pg_tbl_offset, frm->frm_num);
	}
	else{
		// create pd entry as it is absent
		frame_t * pg_tbl = create_pg_tbl(currpid);
		make_pg_dir_entry(pg_dir, pg_dir_offset, pg_tbl->frm_num);
		make_pg_tbl_entry(pg_tbl, pg_tbl_offset, frm->frm_num);
		pg_tbl_frm = pg_tbl->frm_num ;
	}
    return pptr->pdbr;
}

void remove_pg_tbl_entries(frame_t *pg_dir, int vpno, int num_pgs){
	int  i;
	for(i = vpno; i < vpno + num_pgs; i++){
		unsigned long vaddr = i * NBPG;
		unsigned int pd_offset = (vaddr & 0xFFC00000) >> 22;
		unsigned int pt_offset = (vaddr & 0x3FF000) >> 12;
		pd_t *tmp1 = (pd_t *) ((NBPG * pg_dir->frm_num) + pd_offset * sizeof(pd_t));
		if(tmp1->pd_pres == 1){
			frame_t * pg_tbl = get_frm_from_frm_num(tmp1->pd_base);
			remove_pg_tbl_entry(pg_tbl, pt_offset);

		}
	}
	write_cr3(proctab[currpid].pdbr * NBPG);
}

