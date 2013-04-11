/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	char	*block;
	unsigned size;
{
		STATWORD ps;
		if(size == 0)
			return(SYSERR);
		disable(ps);
		mem_list *mem = (mem_list *)(getmem(sizeof(mem_list)));
		mem->mem = block;
		mem->memlen = size;
		struct pentry *pptr = &proctab[currpid];
		mem_list *tmp = &(pptr->mem_list_t);
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = mem;
		restore(ps);
		return(OK);
}
