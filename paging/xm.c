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

  if ( (virtpage < 4096) || ( source < 0 ) || ( source > MAX_ID) ||(npages < 1) || ( npages >200)){
	kprintf("xmmap call error: parameter error! \n");
	return SYSERR;
  }

  if(bs_tab[source].status == BSM_UNMAPPED){
	  bs_tab[source].status = BSM_MAPPED;
	  bs_tab[source].npages = npages;
  }

  struct pentry *pptr = &proctab[currpid];
  bs_map_t *map = &(pptr->map[source]);
  map->bs = source;
  map->pid = currpid;
  map->npages = npages;
  map->vpno = virtpage;
  map->status = BSM_MAPPED;

  bs_t *tab  = &bs_tab[source];
  bs_map_t owner/* = (bs_map_t * )malloc(sizeof(bs_map_t))*/;
  owner.bs = source;
  owner.next = NULL;
  owner.npages = npages;
  owner.pid = currpid;
  owner.vpno = virtpage;
  if(tab->owners == NULL)
	  tab->owners = &owner;
  else{
	  bs_map_t *temp = tab->owners;
	  while(temp->next != NULL)
		  temp = temp->next;
	  temp->next = &owner;
  }
  return OK;
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

  kprintf("To be implemented!");
  return SYSERR;
}

