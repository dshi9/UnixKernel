#ifndef SYSCALL_H
#define SYSCALL_H

#include "file_helper.h"
#include "types.h"
#include "lib.h"


#define U_CS  0x0023
#define	U_DS  0x002B

typedef struct fops
{
	int32_t (*fops_open)(const uint8_t* filename);
	int32_t (*fops_read)(int32_t fd, uint8_t* buf, uint32_t nbytes);
	int32_t (*fops_write)(int32_t fd, const uint8_t* buf, uint32_t nbytes);
	int32_t (*fops_close)(int32_t fd);
} fops_t;

/* file struct used in the PCB */
typedef struct fd_array
{
	fops_t* fops_ptr;		
	inode_t* inode_ptr;	// pointer to inode
	uint32_t fpos;			//file postion 
	uint32_t flags;			//flags
} fd_array_t;

//pcb block 
typedef struct pcb_t
{
	uint32_t pid;
	uint32_t child_process_num;
	uint32_t* prev_pcb;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	fd_array_t fd_array[8];
	uint8_t  arg_buf[128];
}__attribute__((packed)) pcb_t;

extern uint32_t pid_used[6];
extern uint32_t pid_num;
int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, uint8_t* buf, int32_t nbytes);
int32_t write(int32_t fd, const uint8_t* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);
uint32_t get_pid();

#endif
