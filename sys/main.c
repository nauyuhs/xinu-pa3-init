/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();



void test_xmmap(char *name, char *arg){
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = 1;
	int i = ((unsigned long) addr) >> 12;	// the ith page

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return;
	}

	for (i = 0; i < 16; i++) {
		kprintf("Write Operation\n==================\n");
		*addr = 'A' + i;
		addr += NBPG;	//increment by one page each time
	}

	addr = (char*) 0x40000000; //1G

	for (i = 0; i < 16; i++) {
		kprintf("\nRead Operation\n==================\n");
		kprintf("0x%08x: %c\n", addr, *addr);
		addr += 4096;       //increment by one page each time
	}
	xmunmap(0x40000000 >> 12);
}

void test_xmmap_share(char *name, char *arg){
	kprintf("\nthe data written by previous process should be read\n");
	char *addr = (char*) 0x80000000; //1G
	bsd_t bs = 1;
	int i = ((unsigned long) addr) >> 12;	// the ith page
	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return;
//		return SYSERR;
	}

	for (i = 0; i < 16; i++) {
		kprintf("\nRead Operation\n==================\n");
		kprintf("0x%08x: %c\n", addr, *addr);
		addr += 4096;       //increment by one page each time
	}
	xmunmap(0x80000000 >> 12);
//	return OK;
}

void test_vcreate(char *name, char *arg){
	kprintf("\n testing vcreate \n");
	int *x;
	int temp;
	kprintf("\ngetting memory\n");
	x = vgetmem(1000); /* allocates some memory in the virtual heap which is in virtual memory */
	/* the following  statement will cause a page fault. The page fault handling routing  will read in the required page from backing store into the main memory, set the proper page tables and the page directory entries and reexecute the statement. */
	*x = 100;
	x++;
	*x = 200;
	temp = *x; /* You are reading back from virtual heap to check if the previous write was successful */
	kprintf("data read from virtual heap = %d\n", temp);
	kprintf("\nfreeing memory through vgetmem\n");
	vfreemem(--x, 1000); /* frees the allocation in the virtual heap */
}

void test_zero_vgetmem(char *name){
	kprintf("\n testing vcreate \n");
		int *x;
		int temp;
		kprintf("\ngetting memory\n");
		x = vgetmem(0); /* allocates some memory in the virtual heap which is in virtual memory */
		/* the following  statement will cause a page fault. The page fault handling routing  will read in the required page from backing store into the main memory, set the proper page tables and the page directory entries and reexecute the statement. */
		*x = 100;
		x++;
		*x = 200;
		temp = *x; /* You are reading back from virtual heap to check if the previous write was successful */
		kprintf("data read from virtual heap = %d\n", temp);
		kprintf("\nfreeing memory through vgetmem\n");
		vfreemem(--x, 1000); /* frees the allocation in the virtual heap */
}

void frame_share1(char *name, char *arg){
	kprintf("**************executing %s**********\n", name);
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = 1;
	int i = ((unsigned long) addr) >> 12;	// the ith page

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return;
	}

	kprintf("Write Operation for addr = %x\n==================\n", addr);
	*addr = 'A';
	kprintf("value expected at addr %x is 'A' and found :%c\n", addr, *addr);
	kprintf("proc %s going to sleep\n", name);
	sleep(1);
	kprintf("!!!!!!!!!!!!!!calling xmunmap for proc %s\n", name);
	xmunmap(0x40000000 >> 12);

}

void frame_share2(char *name, char *arg){
	kprintf("**************executing %s**********\n", name);
	char *addr = (char*) 0x80000000; //1G
	bsd_t bs = 1;
	int i = ((unsigned long) addr) >> 12;	// the ith page

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return;
	}

	kprintf("Write Operation for addr = %x\n==================\n", addr);
	kprintf("before writing value at addr %x is %c\n", addr, *addr);
	*addr = 'B';
	kprintf("value expected at addr %x is 'B' and found :%c\n", addr, *addr);
	kprintf("!!!!!!!!!!!!!!calling xmunmap for proc %s\n", name);
	xmunmap(0x80000000 >> 12);

}


void test_frame_share(){
	int p1 = create(frame_share1,  2000,  35, "frame_share1", 1, "frame_share1");
	int p2 = create(frame_share2,  2000,  30, "frame_share2", 1, "frame_share2");
	resume(p1);
	resume(p2);
	char *addr = (char *)0x00900000;
	kprintf("value in bs = %c\n", *addr);
}

void test_free_mem(char *name, char *arg){
	kprintf("executing %s\n", name);
	kprintf("before getting memory\n");
	print_free_mem_status();
	kprintf("getting memory\n");
	int *x = vgetmem(1000);
	kprintf("freeing the memory \n");
	vfreemem(x, 1000);
	print_free_mem_status();
}

void test_fifo(char *name, char *arg){
	srpolicy(FIFO);
	kprintf("executing %s, this is tested with 25 frames only. change NFRAMES to 25\n", name);
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = 1;
	int i = ((unsigned long) addr) >> 12;	// the ith page

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return;
	}

	i = 0;
	kprintf("*******************generating first page fault\n");
	*addr = 'A' + (i++);
	addr += NBPG;	//increment by one page each time
	kprintf("************filling all the pages\n");
	while(!is_free_frm_list_empty()){
		*addr = 'A' + (i++);
		addr += NBPG;	//increment by one page each time
	}
	kprintf("*************all pages filled\n");
	kprintf("~~~~~~~~~~generating fifo page fault\n");
	addr += NBPG;	//increment by one page each time
	*addr = 'A' + (i++);
}

void test_aging(char *name, char *arg) {
	kprintf("executing %s, this is tested with 25 frames only. change NFRAMES to 25\n",
			name);
	kprintf("setting aging policy\n");
	srpolicy(AGING);
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = 1;
	int i = ((unsigned long) addr) >> 12;	// the ith page

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("xmmap call failed\n");
		return;
	}

	i = 0;
	kprintf("*******************generating first page fault\n");
	*addr = 'A' + (i++);
	addr += NBPG;	//increment by one page each time
	kprintf("************filling all the pages\n");
	while (!is_free_frm_list_empty()) {
		*addr = 'A' + (i++);
		addr += NBPG;	//increment by one page each time
	}
	kprintf("*************all pages filled\n");
	kprintf("access first page again\n");
	addr = (char*) 0x40000000; //1G
	*addr = 'A';
	kprintf("************generating aging page fault\n");
	addr += ((++i) * NBPG);	//increment by one page each time
	*addr = 'Z';
	kprintf("setting back fifo policy\n");
	srpolicy(FIFO);
}

void test_xmmap_failure(char *name, char *arg){
	kprintf("executing %s\n", name);
	char *addr = (char*) 0x40000000; //1G
	bsd_t bs = 0;
	int i = ((unsigned long) addr) >> 12;	// the ith page

	get_bs(bs, 200);

	if (xmmap(i, bs, 200) == SYSERR) {
		kprintf("Expected:xmmap call failed\n");
		return;
	}
}

void test_vgetmem_over_allocation(char *name){
	kprintf("\n testing test_vgetmem_over_allocation \n");
		int *x;
		int temp;
		kprintf("\ngetting memory\n");
		x = vgetmem(proctab[currpid].vheap_size + 1); /* allocates some memory in the virtual heap which is in virtual memory */
		/* the following  statement will cause a page fault. The page fault handling routing  will read in the required page from backing store into the main memory, set the proper page tables and the page directory entries and reexecute the statement. */
		*x = 100;
		x++;
		*x = 200;
		temp = *x; /* You are reading back from virtual heap to check if the previous write was successful */
		kprintf("data read from virtual heap = %d\n", temp);
		kprintf("\nfreeing memory through vgetmem\n");
		vfreemem(--x, 1000); /* frees the allocation in the virtual heap */

}

void test_vgetmem_max_allocation(char *name){
	kprintf("\n testing %s\n", name);
		int *x;
		int temp;
		kprintf("\ngetting memory\n");
		x = vgetmem(100*4096); /* allocates some memory in the virtual heap which is in virtual memory */
		/* the following  statement will cause a page fault. The page fault handling routing  will read in the required page from backing store into the main memory, set the proper page tables and the page directory entries and reexecute the statement. */
		*x = 100;
		x++;
		*x = 200;
		temp = *x; /* You are reading back from virtual heap to check if the previous write was successful */
		kprintf("data read from virtual heap = %d\n", temp);
		kprintf("\nfreeing memory through vgetmem\n");
		vfreemem(--x, 1000); /* frees the allocation in the virtual heap */
}


int test_vheap()
{
  struct pentry *pptr = &proctab[currpid];

  kprintf("\n\nProcess with virtual heap in backing store %d.\n", pptr->store);

  print_free_mem_status();

  int size = 100; // get 100 bytes in the virtual memory
  char *words = vgetmem(size);
  char *words2 = vgetmem(size);
  char *words3 = vgetmem(size);
  char *words4 = vgetmem(size);

  words[0] = 'a'; words[1] = 'b'; words[2] = 'c'; words[3] = '\0';
  words2[0] = 'd'; words2[1] = 'e'; words2[2] = 'f'; words2[3] = 'g'; words2[4] = '\0';

  kprintf("0x%08x: %s\n", (unsigned)words, words);
  kprintf("0x%08x: %s\n", (unsigned)words2, words2);

  print_free_mem_status();

  vfreemem(words, size);
  print_free_mem_status();

  vfreemem(words3, size);
  print_free_mem_status();

  char *words5 = vgetmem(8000);

  words5[4095] = 'h'; words5[4096] = 'i'; words5[4097] = 'j'; words5[4098] = '\0';

  kprintf("0x%08x: %s\n", (unsigned)&words5[4095], &words5[4095]);

  vfreemem(words4, size);
  print_free_mem_status();

  vfreemem(words2, size);
  print_free_mem_status();


  vfreemem(words5, 8000);
  print_free_mem_status();

  char *words6 = vgetmem(200 * 4096);
  kprintf("#################checking for large space allocation \n");
  if (words6 == SYSERR) {
    kprintf("ERROR: cannot malloc space in virtual heap, the size to get is too large!\n");
    return 0;
  }

  words6[10*4096] = 'k'; words6[10*4096 + 1] = '\0';

  kprintf("0x%08x: %s\n", (unsigned)&words6[10*4096], &words[10*4096]);
  print_free_mem_status();

  return 0;
}


/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main() {

	kprintf("\n\nHello World, Xinu lives\n\n");
	int p1 = create(test_xmmap,  2000,  35, "test_xmmap", 1, "test_xmmap");
//	int p2 = create(test_xmmap_share,  2000, 30, "test_xmmap_shar", 1, "test_xmmap_shar");
//	int p3 = vcreate(test_vcreate, 100,  2000, 30, "test_vcreate", 1, "test_vcreate");
//	int p4 = vcreate(test_free_mem, 100,  2000, 30, "test_free_mem", 1, "test_free_mem");
	int p5 = create(test_fifo,  2000,  35, "test_fifo", 1, "test_fifo");
//	int p6 = create(test_aging,  2000,  35, "test_aging", 1, "test_aging");
//	int p7 = vcreate(test_xmmap_failure, 100,  2000, 30, "xmmap_failure", 1, "xmmap_failure");
//	int p8 = vcreate(test_zero_vgetmem, 100,  2000, 30, "zero_vgetmem", 1, "zero_vgetmem");
//	int p9 = vcreate(test_vgetmem_over_allocation, 100,  2000, 30, "over_allocation", 1, "over_allocation");
//	int p10 = vcreate(test_vgetmem_max_allocation, 100,  2000, 30, "max_allocation", 1, "max_allocation");
//	int p11 = vcreate(test_vheap, 200,  200, 30, "test_vheap", 1, "test_vheap");
//	test_frame_share();
	resume(p1);
//	resume(p2);
//	resume(p3);
//	resume(p4);
	resume(p5);
//	resume(p6);
//	resume(p7);
//	resume(p8);
//	resume(p9);
//	resume(p10);
//	resume(p11);
	return 0;
}
