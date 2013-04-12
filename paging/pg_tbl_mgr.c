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
	write_pg_tbl_entry(frm, idx, 1, 1, base);
}

void remove_pg_tbl_entry(frame_t *frm, int idx){
	write_pg_tbl_entry(frm, idx, 0, 0, 0);
}

void write_pg_tbl_entry(frame_t *frm, int idx, unsigned int pt_pres, unsigned int pt_write, unsigned int base){
	pt_t *tmp = (pt_t *) (NBPG * frm->frm_num);
	tmp += idx;
	pt_t ptr;
	ptr.pt_pres = pt_pres;
	ptr.pt_write = pt_write;
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

void make_pg_dir_entry(frame_t *frm, int idx, int base){
	write_pg_dir_entry(frm, idx,1,1,base);
}

void remove_pg_dir_entry(frame_t *frm, int idx){
	write_pg_dir_entry(frm, idx,0,0,0);
}

void write_pg_dir_entry(frame_t *frm, int idx, unsigned int pd_pres, unsigned int pd_write, unsigned int base){
	pd_t *tmp1 = get_pg_dir_entry(frm, idx);
	pd_t ptr1;
	ptr1.pd_pres = pd_pres;
	ptr1.pd_write = pd_write;
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
	// dont free global pages
	pd_t *ptr1 = get_pg_dir_entry(pd, NUM_GLB_PG_TBLS);
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

unsigned long add_entry_for_pg_fault(int pid, unsigned long vaddr, frame_t * frm ){
	int avail = 0, i;
	unsigned int pg_dir_offset, pg_tbl_offset;
	struct pentry *pptr = &proctab[pid];
	get_offsets_from_vaddr(vaddr, &pg_dir_offset, &pg_tbl_offset);
	pd_t *tmp1 = get_pg_dir_entry(pptr->pd, pg_dir_offset);
	if (tmp1->pd_pres == 1) {
		frame_t *pg_tbl = get_frm_from_frm_num(tmp1->pd_base);
		make_pg_tbl_entry(pg_tbl, pg_tbl_offset, frm->frm_num);
	}
	else{
		// create pd entry as it is absent
		frame_t * pg_tbl = create_pg_tbl(currpid);
		make_pg_dir_entry(pptr->pd, pg_dir_offset, pg_tbl->frm_num);
		make_pg_tbl_entry(pg_tbl, pg_tbl_offset, frm->frm_num);
	}
    return pptr->pdbr;
}

void remove_pg_tbl_entries(frame_t *pg_dir, int vpno, int num_pgs){
	int  i;
	unsigned int pd_offset, pt_offset;
	for(i = vpno; i < vpno + num_pgs; i++){
		get_offsets_from_vaddr(get_vaddr_from_vpno(vpno), &pd_offset, &pt_offset);
		pd_t *tmp1 = get_pg_dir_entry(pg_dir, pd_offset);

		if(tmp1->pd_pres == 1){
			frame_t * pg_tbl = get_frm_from_frm_num(tmp1->pd_base);
			remove_pg_tbl_entry(pg_tbl, pt_offset);
		}
	}
	write_cr3(proctab[currpid].pdbr * NBPG);
}

unsigned long get_vaddr_from_vpno(int vpno){
	return vpno * NBPG;
}

void get_offsets_from_vaddr(unsigned long vaddr, unsigned int *pd_offset, unsigned int *pt_offset){
	*pd_offset = (vaddr & 0xFFC00000) >> 22;
	*pt_offset = (vaddr & 0x3FF000) >> 12;
}

pd_t *get_pg_dir_entry(frame_t *pg_dir, int offset){
	return (pd_t *) ((NBPG * pg_dir->frm_num) + offset * sizeof(pd_t));
}

int has_page_been_accessed(int pid, int vpno){
	unsigned int pd_offset, pt_offset;
	get_offsets_from_vaddr(get_vaddr_from_vpno(vpno), &pd_offset, &pt_offset);
	pd_t *tmp1 = get_pg_dir_entry(proctab[pid].pd, pd_offset);
	if(tmp1->pd_pres){
		pt_t *pt = (pt_t *) (NBPG * tmp1->pd_base);
		pt += pt_offset;
		return pt->pt_acc;
	}
	return 0;
}

void  make_page_access_zero(int pid, int vpno){
	unsigned int pd_offset, pt_offset;
	get_offsets_from_vaddr(get_vaddr_from_vpno(vpno), &pd_offset, &pt_offset);
	pd_t *tmp1 = get_pg_dir_entry(proctab[pid].pd, pd_offset);
	pt_t *pt = (pt_t *) (NBPG * tmp1->pd_base);
	pt += pt_offset;
	pt->pt_acc = 0;
}

