/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main() {
	char *addr = (char*) 0x40000000; //1G
//	char *addr = (char*) (2048*4096);
	bsd_t bs = 1;

	int i = ((unsigned long) addr) >> 12;	// the ith page

	kprintf("\n\nHello World, Xinu lives\n\n");

	pt_t *ptr = (pt_t *)(NBPG*NFRAMES);
	ptr += 1023;
	kprintf("\nAccessing first PT, size %d, base %d\n",sizeof(ptr), ptr->pt_base);
	pd_t *ptr1 = (pd_t *)(NBPG*NFRAMES);
	ptr1 += NBPG;
	kprintf("\nAccessing first PD, size %d, base %d\n",sizeof(ptr1), ptr1->pd_base);
	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
//		return 0;
	}

	for (i = 0; i < 16; i++) {
		*addr = 'A' + i;
		addr += NBPG;	//increment by one page each time
	}

	addr = (char*) 0x40000000; //1G
	for (i = 0; i < 16; i++) {
		kprintf("0x%08x: %c\n", addr, *addr);
		addr += 4096;       //increment by one page each time
	}
//
	xmunmap(0x40000000 >> 12);

	addr = (char*) 0x901000;
	kprintf("val in bs 1 pg 0 %c\n", *addr);
	return 0;
}
