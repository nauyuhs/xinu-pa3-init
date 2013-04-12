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
	add_mapping_to_proc_frm_list(frm, store, currpid);
	write_cr3(pdbr * NBPG);
	restore(ps);
	kprintf("faulting on addr %x for proc %d and cr3 = %d\ and page  = %d and vpno = %d\n",
			vaddr, currpid, read_cr3(), pageth, frm->fr_vpno );
//	kprintf("assigned frm num %d for addr %x\n", frm->frm_num, vaddr);
	return OK;
}


