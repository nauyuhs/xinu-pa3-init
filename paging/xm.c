/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  /* sanity check ! */

	if (get_bs(source, npages) == SYSERR) {
		kprintf("error in get_bs\n");
		return SYSERR;
	}
	if ((virtpage < 4096) || (source < 0) || (source > MAX_ID) || (npages < 1)
			|| (npages > 200)) {
		kprintf("xmmap call error: parameter error! \n");
		return SYSERR;
	}
  kprintf("map bs%d into process %d:%x[%d]\n", source, currpid, virtpage, virtpage);

  return bsm_map(currpid, virtpage, source, npages);
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage )
{
  /* sanity check ! */
  if ( (virtpage < 4096) ){ 
	kprintf("xmummap call error: virtpage (%d) invalid! \n", virtpage);
	return SYSERR;
  }
  kprintf("unmap process %d:%x[%d]", currpid, virtpage, virtpage);

  bsm_unmap(currpid, virtpage, 1);

  write_cr3(proctab[currpid].pdbr * NBPG);
  return OK;
}

