#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */

	STATWORD ps; 

	disable(ps);

	if((npages == 0 || npages > 200) || (bs_id < 0 || bs_id > MAX_ID) ||bs_tab[bs_id].as_heap) {
		restore(ps);
		return(SYSERR);
	}
//	kprintf("Calling get_bs for bs: %d npages:%d\n", bs_id, npages);
//	kprintf("%d pages of %d bs allocated\n", npages, bs_id);
	restore(ps);
    return npages;

}


