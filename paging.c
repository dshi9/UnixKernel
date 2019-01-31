#include "paging.h"
#include "lib.h"

#define page_padding			0x00400000
#define flag_mask				0x3ff
#define video_mem_PT_index		255
#define	VID_MEM_ADDR			0xB8000
#define VID_MEM_0 0x1000
#define VID_MEM_1 0x3000
#define VID_MEM_2 0x5000
//global page directory/table with 1024 entries and 4B per entry
uint32_t page_dir[1024] __attribute__((aligned(4096)));
uint32_t page_table_1[1024] __attribute__((aligned(4096)));
uint32_t user_video_PT[1024] __attribute__((aligned(4096)));
/*
* init_paging
*   DESCRIPTION: This function initialize page directory and page table.
*				 link a page table to first entry of PDE and a 4MB page to the second entry
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: initialize page directory and a page table
*				  and map 0-8 MB memory into page directory
*/
//uint32_t page_table_1_big[1024*1024] __attributte__((aligned(4096*1024)));
void
init_paging() {
	int i;
	unsigned j;
	/*traverse all the 1024 entries of page directory*/
	for (i = 0; i < 1024; i++) {
		/*only kernel accessible(0), write enabled(1), not present(0)*/
		page_dir[i] = 0x00000002;
	}
	/*traverse all the 1024 entries of page table*/
	for (j = 0; j < 1024; j++) {
		/*video memory address*/
		if ((j * 0x1000) == VID_MEM_ADDR || (j * 0x1000) == VID_MEM_0 || (j * 0x1000) == VID_MEM_1 || (j * 0x1000) == VID_MEM_2) {
			/*the last 12 bits is for other use so we skip that and only kernel accessible(0), write enabled(1), present(1)*/
			page_table_1[j] = (j * 0x1000) | 3;
		}
		else {
			/*the last 12 bits is for other use so we skip that and only kernel accessible(0), write enabled(1), present(0)*/
			page_table_1[j] = (j * 0x1000) | 2;
		}
	}

	/*set the first entry of page driectory to the address of page_table_1*/
	/*the entry 20 most significant bit are the address since it should be multiple of 4096 btyes*/
	page_dir[0] = ((unsigned int)page_table_1) | 3;

	/*map at 4 MB set the 8th bit as 1 to enable 4MB page. only kernel accessible(0), write enabled(1), present(1)*/
	page_dir[1] = (0x400000) | (0x83);

	//load page_dir address
	load_page_dir(page_dir);
}

/*
* page_map
*   DESCRIPTION: Map virtual memory to physical memory
*   INPUTS: virtual_addr -- virtuall address 
*			phys_addr	 -- physical address
*			flags		 -- flags for PDE
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: change PDE
*/
void page_map(uint32_t virtual_addr, uint32_t phys_addr, uint32_t flags) {
	int pdt_index = virtual_addr >> 22;

	page_dir[pdt_index] = phys_addr | (flag_mask&flags);

	load_page_dir(page_dir);
}

/*
* video_page_map
*   DESCRIPTION: Map video memory to specified virtual memory
*   INPUTS: virtual_addr -- virtuall address
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: change PDE
*/
void video_page_map(uint32_t virtual_addr) {
	int i;
	int pdt_index = virtual_addr >> 22;
	//initialize 1024 entries in video memory page table
	for (i = 0; i<1024; i++)
		//enable the first 4kB page : user (1), write enabled(1), present(0)
		user_video_PT[i] = 0x00000006;
	//set the page directory entry for user video memory: user (1), write enabled(1), present(1)
	page_dir[pdt_index] = ((unsigned int)user_video_PT) | 0x7;
	//enable the first 4kB page : user (1), write enabled(1), present(1)
	user_video_PT[0] = VID_MEM_ADDR | 0x7;
	return;
}


/*
* load_page_dir
*   DESCRIPTION: This function load corresponding CR3 value which is base for page directory
*   INPUTS: page_directory -- the address of page directory will be loaded into CR3
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: change the value of CR3 to the address of page_directory
*/
void
load_page_dir(uint32_t* page_directory) {
	asm("movl %0, %%eax	;				\
		 movl %%eax, %%cr3				\
		":								\
		: "r"(page_directory)			\
		: "%eax"						\
		);
}

/*
void
enable_paging() {
unsigned int a = 0x80000000;
asm("movl %%cr0, %%eax;			\
orl %0, %%eax;				\
movl %%eax, %%cr0			\
":							\
:"r"(a)					\
:"%eax"					\
);
}

void
enable_extended_page() {
asm("movl %%cr4, %%eax;			\
orl $0x10, %%eax;			\
movl %%eax, %%cr4			\
":						\
:						\
:"%eax"					\
);
}
*/
