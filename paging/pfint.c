/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint() {

	STATWORD ps;
	disable(ps);
	int store, pageth;
	unsigned long vaddr = read_cr2();
	bsm_lookup(currpid, vaddr, &store, &pageth);
	frame_t * frm = bs_get_frame(store, pageth);
	unsigned int pg_dir_offset = (vaddr & 0xFFC00000) >> 22;
	unsigned int pg_tbl_offset = (vaddr & 0x3FF000) >> 12;
	unsigned long pdbr = add_pg_dir_entry_for_pg_fault(currpid, pg_dir_offset, pg_tbl_offset, frm);
//	write_cr3(pdbr * NBPG);
	restore(ps);
	pd_t *ptr = (pd_t *) ((1029 * NBPG) );
	pt_t *ptr1 = (pt_t *) ((1031 * NBPG));
//	kprintf("assigned frm num %d for addr %x\n", frm->frm_num, vaddr);
	return OK;
}


