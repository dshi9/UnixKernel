#include "syscall.h"
#include "rtc.h"
#include "terminal.h"
#include "file_helper.h"
#include "lib.h"
#include "x86_desc.h"
#include "paging.h"
#include "keyboard.h"

#define LARGE_PAGE_FLAG 0x87
#define V_BASE_ADDR		0x08000000//128MB
#define V_BASE_OFFSET	0x00048000
#define P_BASE_ADDR		0x00800000//8MB
#define LARGE_PAGE_PAD	0x00400000
#define PCB_PAD			0x2000//8kb
#define PAGE_PAD		0x1000//8kb
#define USER_VID_MEM	0x40000000//1024MB for user video memory
#define	VID_MEM			0xB8000
#define eight_kb 0x2000
#define ONE_SPACE 4
#define cmd_length 128
#define buf_length 128
#define fd_max 8
#define fd_stdout 1
#define	MAX_PROCESS 6
#define	EXE_EXCEPTION 256
fops_t stdin_fops = { &terminal_open, &terminal_read, NULL, &terminal_close };
fops_t stdout_fops = { &terminal_open, NULL, &terminal_write, &terminal_close };
fops_t rtc_fops = { &rtc_open, &rtc_read, &rtc_write, &rtc_close };
fops_t file_fops = { &file_open, &file_read, &file_write, &file_close };
fops_t dir_fops = { &dir_open, &dir_read, &dir_write, &dir_close };
uint32_t pid_num = 0;//current program we have
uint32_t pid_used[6];
///////////////////////////////////////////////////////////////
/*	halt
*	DESCRIPTION: System call for halt. Halt current program 
*				 and restore ,ebp, tss.esp0, tss.ss0.
*				 Then back to parent program if there is any
*	INPUT: status	-- halt status (??)
*	OUTPUT: halt current program and return 0. Change value in current ebp,esp 
*		    and remap memory to parent program if this is not last one
*	RETURN: 0 on success
*	SIDE EFFECTS: none
*/
///////////////////////////////////////////////////////////////
int32_t halt(uint8_t status)
{
	int i;
	/*get pcb for program to be halted*/
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);
	pid_used[pid_num-1] = 0;
	/*close all 8 file*/
	for (i = 0; i<fd_max; i++)
		close(i);

	/*retore pid for parent process*/
	if(pcblock->prev_pcb!= NULL){
		pcb_t* prev_pcblock = (pcb_t*)(pcblock->prev_pcb);
		pid_num = prev_pcblock->pid;
	}
	else{
		term[terminal_idx].running = 0;
		pid_num = 0;
	}
	(term[terminal_idx].process_num)--;
	
	asm volatile (
		"MOVL %0, %%ebp;"
		"MOVL %1, %%esp;"
		:
	: "r"(pcblock->ebp), "r"(pcblock->esp)
		: "esp", "ebp", "memory", "cc" //clobbered register 
		);
	
	tss.esp0 = P_BASE_ADDR - (PCB_PAD*pid_num) - ONE_SPACE;
	tss.ss0 = KERNEL_DS;

	/*restore parents paging*/
	if (pid_num>0)
		page_map(V_BASE_ADDR, P_BASE_ADDR + (pid_num - 1)*LARGE_PAGE_PAD, LARGE_PAGE_FLAG);
	
	asm volatile (
		"JMP HALT_LABEL;"//jump to halt return label
		:
	: "r"(status)
		: "memory", "cc"
		);

	return 0;
}

///////////////////////////////////////////////////////////////
/*	int32_t execute(const uint8_t* command)
*	DESCRIPTION: System call for execute. execute the program by name indicated by command
*	INPUT: command	-- file descriptor
*	OUTPUT: execute a prgram in terminal if we find such exe file in file system.
*		    Otherwise, we return -1.
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: none
*/
///////////////////////////////////////////////////////////////
int32_t execute(const uint8_t* command)
{

	/*use pid_num to record which is current process*/

	/*parse command*/
	dentry_t dentry_struct;
	dentry_t* dentry_read = &(dentry_struct);
	
	/*save the argument in the pcb block */
	int cmd_idx = 0; // looping index for command
	uint8_t first_sp = 0;
	uint8_t arg_off = 0;
	uint8_t exe_cmd[cmd_length];
	uint8_t argument_buffer[buf_length];
	while(command[cmd_idx] != '\0')
	{
		if(command[cmd_idx] == ' ' && first_sp == 0)
		{
			exe_cmd[cmd_idx] = '\0';
			first_sp = 1;
			arg_off = cmd_idx+1;
		}
		//taking the argument
		else if(first_sp == 1)
			argument_buffer[cmd_idx-arg_off] = command[cmd_idx];

		else
			exe_cmd[cmd_idx] = command[cmd_idx];
		cmd_idx++;

	}
	argument_buffer[cmd_idx-arg_off] = '\0';
	if(first_sp == 0)
	{
		exe_cmd[cmd_idx] = '\0';
	}

	if (0 != read_exec_dentry_by_name(exe_cmd, dentry_read))
		return -1;

	/*check if exec file. If not exe, return -1*/
	uint8_t data_buf[ONE_SPACE];
	int32_t test_bit = read_data(dentry_read->inode_num, 0, data_buf, 4);
	/*check if we get 4 bytes to test magic number*/
	if (test_bit != ONE_SPACE)
		return -1;
	/*test magic number for executable*/
	if (data_buf[0] != 0x7f || data_buf[1] != 0x45 || data_buf[2] != 0x4c || data_buf[3] != 0x46)
		return -1;

	uint32_t prev_pid = pid_num;
	/*calculate current pid*/
	if (term[terminal_idx].process_num > 3)
		return EXE_EXCEPTION;
	int idx;
	for (idx = 0; idx < MAX_PROCESS; idx++) {
		/*if find available pid num. Mark it as used*/
		if (pid_used[idx] == 0) {
			pid_used[idx] = 1;
			pid_num = idx + 1;
			(term[terminal_idx].process_num)++;
			break;
		}
		if (idx == MAX_PROCESS-1)
			return EXE_EXCEPTION;
	}
	/*update current pid for current terminal*/
	term[terminal_idx].cur_pid = pid_num;

	/*locate current pcb_block*/
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);
	/*store pid for current pcb*/
	pcblock->pid = pid_num;
	/*store previous pcb*/
	if (prev_pid > 0)
		pcblock->prev_pcb = (uint32_t *)(P_BASE_ADDR - prev_pid*PCB_PAD);
	else
		pcblock->prev_pcb = NULL;
	//check whether this user program is shell or not
	if(strncmp((int8_t*)"shell", (int8_t*)(command), 5)==0)
	{
		if (term[terminal_idx].running == 0) {
			term[terminal_idx].running = 1;
			term[terminal_idx].term_pid = pid_num;
			pcblock->prev_pcb = NULL;
		}
	}

	strcpy((int8_t*) pcblock->arg_buf, (const int8_t*) argument_buffer);

	/*setup paging.*/
	page_map(V_BASE_ADDR, P_BASE_ADDR + (pid_num - 1)*LARGE_PAGE_PAD, LARGE_PAGE_FLAG);
	/*file load. copy file into physical memory*/
	/*get eip from 24-27 bytes in file*/
	uint8_t file_eip[ONE_SPACE];
	read_data(dentry_read->inode_num, 24, file_eip, 4);
	/*load file into vitual address in page with offset*/
	uint8_t* file_data = (uint8_t*)(V_BASE_ADDR + V_BASE_OFFSET);
	inode_t* inode_read = (inode_t*)(inode_start + dentry_read->inode_num);
	read_data(dentry_read->inode_num, 0, file_data, inode_read->length);

	/*setup PCB*/
	/*open stdin.set flag as 1 indicating use*/
	pcblock->fd_array[0].fops_ptr = &stdin_fops;
	pcblock->fd_array[0].inode_ptr = NULL;
	pcblock->fd_array[0].fpos = 0;
	pcblock->fd_array[0].flags = 1;
	/*open stdout.set flag as 1 indicating use*/
	pcblock->fd_array[1].fops_ptr = &stdout_fops;
	pcblock->fd_array[1].inode_ptr = NULL;
	pcblock->fd_array[1].fpos = 0;
	pcblock->fd_array[1].flags = 1;

	/* set kernel stack */
	tss.esp0 = P_BASE_ADDR - (PCB_PAD*pid_num) - ONE_SPACE;
	tss.ss0 = KERNEL_DS;
	/*initialize other things in PCB*/
	/* store the current esp and ebp into the PCB*/
	uint32_t cur_esp;
	uint32_t cur_ebp;
	asm volatile ("movl %%esp, %0":"=g"(cur_esp));//??
	asm volatile ("movl %%ebp, %0":"=g"(cur_ebp));
	pcblock->esp = cur_esp;
	pcblock->ebp = cur_ebp;
	/*
	term[terminal_idx].esp = cur_ebp;
	term[terminal_idx].ebp = cur_esp;
	*/
	/*context switch*/
	uint32_t entry_eip = 0;
	int i;
	/*use 4 bytes from file 24-27 bytes to get eip*/
	for (i = 0; i<ONE_SPACE; i++)
		entry_eip = entry_eip | (file_eip[i] << (8 * i));//shift 8 bit each time to construct a 32 bytes eip value
	
	int32_t val;

	//uint32_t esp_value = USER_PROGRAM_ESP_START; 
	asm volatile (
		"MOVW $0x2B, %%BX;"
		"MOVW %%BX, %%DS;"
		"MOVW %%BX, %%ES;"
		"PUSHL %1;"
		"PUSHL $0x83FFFFC;"//128+4MB starting location 
		"PUSHF;"
		"PUSHL %2;"
		"PUSHL %3;"
		"IRET;"
		"HALT_LABEL:"
		: "=r"(val)
		: "r"(U_DS), "r"(U_CS), "r"(entry_eip)
		: "ebx"
		);

//	return val;
	term[terminal_idx].cur_pid = pid_num;
	/*user cannot exit last "shell" program. If so, execute shell again*/
	if (term[terminal_idx].running == 0)
		execute((uint8_t*)"shell");
	return 0;
}

///////////////////////////////////////////////////////////////
/*	read
*	DESCRIPTION: Call read function through file operation table
*	INPUT: fd		-- file descriptor index
*		   buf		-- the buffer to read
*		   nbytes	-- how many bytes to read
*	OUTPUT: data buffer
*	RETURN: 0 on success, -1 fail
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t read(int32_t fd, uint8_t* buf, int32_t nbytes)
{
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);
	/*check if buf is valid and if fd is within valid range. Also file read in fd_array should has flags set 1*/
	if (buf == NULL || fd < 0 || fd >= fd_max || pcblock->fd_array[fd].flags == 0 || nbytes < 0)
		return -1;

	if (fd == 1)
		return -1;//you cant read in stdout

	int32_t read_bytes = pcblock->fd_array[fd].fops_ptr->fops_read(fd, buf, nbytes);
	return read_bytes;
}

///////////////////////////////////////////////////////////////
/*	write
*	DESCRIPTION: Call write function through file operation table
*	INPUT: fd		-- file descriptor index
*		   buf		-- the buffer to write 
*		   nbytes	-- how many bytes to write 
*	OUTPUT: none
*	RETURN: 0 on success, -1 fail
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t write(int32_t fd, const uint8_t* buf, int32_t nbytes)
{
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);
	/*check if buf is valid and if fd is within valid range. Also file write in fd_array should has flags set 1*/
	if (buf == NULL || fd < 0 || fd >= fd_max || pcblock->fd_array[fd].flags == 0 || nbytes < 0)
		return -1;

	if (fd == 0)
		return -1;//you cant write in stdin

	int32_t write_bytes;
	//printf("got here\n");
	write_bytes = pcblock->fd_array[fd].fops_ptr->fops_write(fd, buf, nbytes);
	return write_bytes;
}

///////////////////////////////////////////////////////////////
/*	int32_t open(const uint8_t* filename)
*	DESCRIPTION: System call for open. Use the available file descriptor. 
*				 Set function pointer table and inode pointer for file
*	INPUT: int8_t* filename -- the file we want to open and store in fd_array
*	OUTPUT: none
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: add rtc/dir/file in fd_array
*/
///////////////////////////////////////////////////////////////
int32_t open(const uint8_t* filename)
{
	//test if string pointer is valid and if it is a empty string
	if(filename == NULL || filename[0] == 0)
		return -1;

	int i;
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);
	dentry_t dentry_struct;
	dentry_t* dentry_read = &dentry_struct;
	if (read_dentry_by_name(filename, dentry_read) == -1)
		return -1;//no such file
	
	/*we should find available slot from 2*/
	for (i = 2; i < fd_max; i++) {
		if (pcblock->fd_array[i].flags == 0) {
			/*set this file descriptor as used and initialize file position*/
			pcblock->fd_array[i].flags = 1;
			pcblock->fd_array[i].fpos = 0;
			/*check file type 0, 1, 2*/
			/*file type is a file giving user-level access to RTC*/
			if (dentry_read->filetype == 0) {
				pcblock->fd_array[i].fops_ptr = &rtc_fops;
				pcblock->fd_array[i].inode_ptr = NULL;
			}
			/*file type is a directory*/
			if (dentry_read->filetype == 1) {
				pcblock->fd_array[i].fops_ptr = &dir_fops;
				pcblock->fd_array[i].inode_ptr = NULL;
			}
			/*file type is regular file*/
			if (dentry_read->filetype == 2) {
				pcblock->fd_array[i].fops_ptr = &file_fops;
				pcblock->fd_array[i].inode_ptr = (inode_t*)(dentry_read->inode_num + inode_start);
			}
			break;
		}

		/*if no usable file descriptor, return -1*/
		if (i == (fd_max-1) && pcblock->fd_array[i].flags != 0)
			return -1;
	}

	/*if index in fd_array*/
	return i;
}

///////////////////////////////////////////////////////////////
/*	close
*	DESCRIPTION: System call for close. Clear valid entry in fd_array.
*	INPUT: fd	-- the index for fd to be closed
*	OUTPUT: Clear entry in file decriptor array
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: close file descriptor with index fd
*/
///////////////////////////////////////////////////////////////
int32_t close(int32_t fd)
{
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);

	/*check if fd is valid and use flag is set to 1,you can't clost stdin stdout*/
	if(pid_used[pid_num-1] == 1){
		if (fd <= fd_stdout || fd >= fd_max || pcblock->fd_array[fd].flags == 0)
			return -1;
	}
	
	pcblock->fd_array[fd].flags = 0;
	pcblock->fd_array[fd].inode_ptr = NULL;
	pcblock->fd_array[fd].fpos = 0;
	pcblock->fd_array[fd].fops_ptr = NULL;
	return 0;
}

///////////////////////////////////////////////////////////////
/*	getargs
*	DESCRIPTION: get args for current programs
*	INPUT: buf		-- buf to store args
*		   nbytes	-- length btyes to store
*	OUTPUT: store args from pcblock in buf
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: close file descriptor with index fd
*/
///////////////////////////////////////////////////////////////
int32_t getargs(uint8_t* buf, int32_t nbytes)
{
	if( buf == NULL || nbytes == 0)
		return -1;
	// get the pcb pointer 
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_num*PCB_PAD);

	// if the arguments do not fit in the buffer, is a error
	uint32_t arg_len = strlen((const int8_t*)pcblock->arg_buf);
	if( arg_len > nbytes )
		return -1;

	// copy the buffer to the user space
	strcpy((int8_t*)buf, (const int8_t*)pcblock->arg_buf);

	return 0;
}

///////////////////////////////////////////////////////////////
/*	vidmap
*	DESCRIPTION: Maps the text-mode video memory into user space at a pre-set virtual memory
*	INPUT: screen_start -- where videomemory for user will be store
*	OUTPUT: (*screen_start) is the video memory starting memory
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: change the address to 
*/
///////////////////////////////////////////////////////////////
int32_t vidmap(uint8_t** screen_start)
{
	/*check if screen_start is valid*/
	if ((uint32_t)screen_start < V_BASE_ADDR || (uint32_t)screen_start >= V_BASE_ADDR + LARGE_PAGE_PAD)
		return -1;
	/*initialize page and map video memory in address defined*/
	video_page_map(USER_VID_MEM);
	*screen_start = (uint8_t*)(USER_VID_MEM);
	return 0;
}

///////////////////////////////////////////////////////////////
/*	set_handler
*	DESCRIPTION:
*	INPUT: signum			--
*		   handler_address	--
*	OUTPUT:  
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: 
*/
///////////////////////////////////////////////////////////////
int32_t set_handler(int32_t signum, void* handler_address)
{
	return 0;
}

///////////////////////////////////////////////////////////////
/*	sigreturn
*	DESCRIPTION:
*	INPUT: none
*	OUTPUT:  
*	RETURN: 0 on success, -1 otherwise
*	SIDE EFFECTS: close file descriptor with index fd
*/
///////////////////////////////////////////////////////////////
int32_t sigreturn(void)
{
	return 0;
}

///////////////////////////////////////////////////////////////
/*	get_pid
*	DESCRIPTION: get current pid number
*	INPUT: none
*	OUTPUT: none
*	RETURN: the current pid number
*	SIDE EFFECTS: 
*/
///////////////////////////////////////////////////////////////
uint32_t get_pid() {
	return pid_num;
}
