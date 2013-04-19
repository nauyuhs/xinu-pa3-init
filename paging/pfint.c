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

//	kprintf("#PF in %s, cr2:%x:ec2\n", proctab[currpid].pname, read_cr2());
	STATWORD ps;
	disable(ps);
	int store, pageth;
	unsigned long vaddr = read_cr2();
	bsm_lookup(currpid, vaddr, &store, &pageth);
	frame_t * frm = bs_get_frame(store, pageth);
	unsigned long pdbr = add_entry_for_pg_fault(currpid, vaddr, frm);
	add_mapping_to_proc_frm_list(frm, store, currpid);
	write_cr3(pdbr * NBPG);
	restore(ps);
	return OK;
}


