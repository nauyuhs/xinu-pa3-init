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
		mem->mem = (char *)block;
		mem->memlen = size;
		mem->next = NULL;
		struct pentry *pptr = &proctab[currpid];
		mem_list *tmp = &(pptr->mem_list_t);
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = mem;
		restore(ps);
		return(OK);
}

/**
 * for testing purpose
 */
void print_free_mem_status(){
	kprintf("\n*****************status of virtual heap***********************\n");
	struct pentry *pptr = &proctab[currpid];
	mem_list *tmp = &(pptr->mem_list_t);
	while (tmp != NULL) {
		if(tmp->memlen != 0){
			kprintf("block starts at addr [%x] with length = %x\n\n", tmp->mem, tmp->memlen);
		}
		tmp = tmp->next;
	}
}
