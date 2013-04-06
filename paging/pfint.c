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
	restore(ps);
	kprintf("store = %d and pageth = %d and allocated frame is %d\n", store , pageth, frm->frm_num);
	return OK;
}


