/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

void producer(char *name, char *dummy) {
	int *x;
	int temp;
	x = vgetmem(1000); /* allocates some memory in the virtual heap which is in virtual memory */
	/* the following  statement will cause a page fault. The page fault handling routing  will read in the required page from backing store into the main memory, set the proper page tables and the page directory entries and reexecute the statement. */

	kprintf("%s makes an attempt to write at %d \n", name, x);
	*x = 100;
	x++;
	*x = 200;
	temp = *x; /* You are reading back from virtual heap to check if the previous write was successful */
	vfreemem(--x, 1000); /* frees the allocation in the virtual heap */
	kprintf("memory freed\n");
    kprintf("%s completes\n", name);
}


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

//	pt_t *ptr = (pt_t *)(NBPG*NFRAMES);
//	ptr += 1023;
//	kprintf("\nAccessing first PT, size %d, base %d\n",sizeof(ptr), ptr->pt_base);
//	pd_t *ptr1 = (pd_t *)(NBPG*NFRAMES);
//	ptr1 += NBPG;
//	kprintf("\nAccessing first PD, size %d, base %d\n",sizeof(ptr1), ptr1->pd_base);
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
//		kprintf("0x%08x: %c\n", addr, *addr);
		addr += 4096;       //increment by one page each time
	}
//
	xmunmap(0x40000000 >> 12);

//	char *kk = (char *)0x40000000;
//	unsigned long adder  = (unsigned)kk + 0x1000;
//	kprintf("vadder physical addr = %d\n", adder);

	int p1 = vcreate(producer,  2000, 100, 30, "producer", 1, "dummyarg");
	 resume(p1);
//	addr = (char*) 0x901000;
//	kprintf("val in bs 1 pg 0 %c\n", *addr);
//	pt_t *ptt = (pt_t *)(1031* 4096);
//	kprintf("pg aftr deletion %d\n", (ptt + 1)->pt_base);
//	pd_t *pdd = (pd_t *)(NBPG * 1029);
//	kprintf("pg dir val %d and pres = %d\n", (pdd+ 256)->pd_base, (pdd+ 256)->pd_pres);
	return 0;
}
