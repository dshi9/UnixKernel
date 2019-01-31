/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "rtc.h"
#include "keyboard.h"
#include "exception.h"
#include "idt.h"
#include "paging.h"
#include "terminal.h"
#include "file_helper.h"
#include "syscall.h"
/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */

#define INIT_RTC 1
#define INIT_KB 1
#define RTC_READ_WRITE_TEST 0
#define TM_READ_WRITE_TEST 0
#define FS_READ_TEST		0

void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;
#if (TEST_PAGING == 1)
	uint32_t* address;
	uint32_t  content;
#endif

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Init the PIC */
	i8259_init();
	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */

	//initializing the IDT table
	idt_init();
  
    

	//initialize keyboard device.
#if (INIT_KB == 1)
	keyboard_init();
#endif
	clear();
	init_cursor();
	//initialize rtc device, the rtc is connected on slave, irq8 (port0x28)
#if (INIT_RTC == 1)
	rtc_init();
	//test for rtc
    
#endif

	//initialize filesystem
	module_t* mod_1 = (module_t*)mbi->mods_addr;
	boot_block = (bootblock*)(mod_1->mod_start);
	inode_start = (inode_t*)(boot_block + 1);
	/*1 for read data from file*/
#if(FS_READ_TEST == 1)
	//	int32_t test_read = read_dentry_by_index(1, dentry);
	int32_t fs_fd = 0;
	uint8_t fname[32] = "frame0.txt"; // 32 is max num of character for file name
	uint32_t length_read = 187;//187 is for test
	uint8_t buf[length_read];
	//uint32_t test_i;
	if (read_data_test(fname, buf, length_read) != -1) {
		terminal_write(fs_fd, buf, length_read);
	}
#endif 

	/* 2 for read data length*/
#if(FS_READ_TEST == 2)
	//	int32_t test_read = read_dentry_by_index(1, dentry);
	uint8_t fname[32] = "frame0.txt"; // 32 is max num of character for file name
	dentry_t* dentry;
	if (read_dentry_by_name(fname, dentry) != -1) {
		inode_t* inode_read = (inode_t*)(inode_start + dentry->inode_num);
		printf("%d", inode_read->length);
	}
#endif 

	/*3 for print all file name in directory*/
#if(FS_READ_TEST == 3)
	uint8_t buf[32];//32 for max char in file name
	int32_t fs_fd = 0;
	int temp_dir = dir_read(fs_fd, buf, 32);
#endif 

	/* 4 for test file_read*/
#if(FS_READ_TEST == 4)
	uint32_t length_read = 100;
	uint32_t index_to_read = 0;
	uint8_t buf[length_read];//32 for max char in file name
	int temp_dir = file_read(index_to_read, buf, length_read);
	int i;
	dentry_t* dentry;
	temp_dir = file_read(index_to_read, buf, length_read);
	terminal_write(index_to_read, buf, length_read);
#endif

	init_paging();
	load_page_dir(page_dir);
	enableExPaging();
	enablePaging();
	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	//printf("Enabling Interrupts\n");
	sti();
	//test for RTC READ WRITE
	#if(RTC_READ_WRITE_TEST == 1)
	
	int32_t rtc_test=16;
	rtc_write(temp_fd,&rtc_test,4);
	int rtc_i=0;
	for (rtc_i=0;rtc_i<2048;rtc_i++)
	{
		rtc_read(temp_fd,&rtc_test,0);
		putc('0');
	}
	/*rtc_write(temp_fd,&rtc_test,256);
	for (rtc_i=0;rtc_i<2048;rtc_i++)
	{
		rtc_read(temp_fd,&rtc_test,0);
		putc('0');
	}
	*/
	
	//test for rtc open and close
	
	//printf("value of open rtc is %d\n", rtc_open(NULL));
	//printf("value of close rtc is %d\n", rtc_close(NULL));
	

	#endif
	
	#if(TM_READ_WRITE_TEST == 1)
	uint8_t test[128] ={'h','\n','l','\n','o','\n','w','o','\n','l','d','\n',
						'h','e','\n','l','o','\n','w','o','\n','l','d','\n',
						'h','e','l','l','o','\n','w','o','r','l','d','\n',
						'h','\n','l','l','o','\n','w','o','\n','l','d','\n',
						'h','e','\n','l','o','\n','w','o','r','l','d','\n',
						'h','e','\n','l','o','\n','w','o','\n','l','d','\n',
						'h','e','\n','l','o','\n','w','\n','\n','l','d','\n',
						'h','e','l','l','\n','\n','w','\n','r','l','d','!'};
	uint8_t* test_buf_ptr;
	test_buf_ptr = terminal_read(temp_fd, test, 84);
	terminal_write(temp_fd, test_buf_ptr, 84);
	#endif
	/*define vidmem_addr for each terminal*/
	term[0].font_color = ATTRIB_TM;
	term[0].vidmem_addr = (char*)VIDEO_SAVE_0;
	term[1].font_color = ATTRIB_TM_1;
	term[1].vidmem_addr = (char*)VIDEO_SAVE_1;
	term[2].font_color = ATTRIB_TM_2;
	term[2].vidmem_addr = (char*)VIDEO_SAVE_2;
	term[0].running = 0;
	term[1].running = 0;
	term[2].running = 0;
	/*initialize process used array*/
	int idx;
	for (idx = 0; idx < 6; idx++) {
		pid_used[idx] = 0;
	}
	//clear all the video buffer
	init_all_video_buf();
	/* Execute the first program ('shell') ... */
	execute((uint8_t*)"shell");
	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

