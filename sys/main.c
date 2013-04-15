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


/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main() {

	kprintf("\n\nHello World, Xinu lives\n\n");
//	int p1 = create(test_xmmap,  2000,  35, "test_xmmap", 1, "test_xmmap");
//	int p2 = create(test_xmmap_share,  2000, 30, "test_xmmap_shar", 1, "test_xmmap_shar");
//	int p3 = vcreate(test_vcreate, 100,  2000, 30, "test_vcreate", 1, "test_vcreate");
//	int p4 = vcreate(test_free_mem, 100,  2000, 30, "test_free_mem", 1, "test_free_mem");
//	int p5 = create(test_fifo,  2000,  35, "test_fifo", 1, "test_fifo");
//	int p6 = create(test_aging,  2000,  35, "test_aging", 1, "test_aging");
	int p7 = vcreate(test_xmmap_failure, 100,  2000, 30, "xmmap_failure", 1, "xmmap_failure");
//	test_frame_share();
//	resume(p1);
//	resume(p2);
//	resume(p3);
//	resume(p4);
//	resume(p5);
//	resume(p6);
	resume(p7);
	return 0;
}
