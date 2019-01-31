#include "types.h"
#include "lib.h"

#ifndef _FILE_HELPER_H
#define _FILE_HELPER_H

typedef struct dentry_t {
	uint8_t	filename[32]; // file name allow up to 32 character
	int32_t filetype;
	int32_t	inode_num;
	int32_t	pad[6];//6 * 4B = 24B reserved
}__attribute__((packed)) dentry_t;

typedef struct inode {
	int32_t	length;
	int32_t	block_num[1024-1];	//4kb have 1024*4B. So we could have up to 1024-1 block num
}__attribute__((packed)) inode_t;

typedef struct bootblock {
	int32_t		num_dir;
	int32_t		num_inode;
	int32_t		num_data;
	int32_t		pad[13];//13 * 4B = 52B reserved
	dentry_t	dir_itself;
	dentry_t	dir_entry[62]; // 62 dir entries allowed
}__attribute__((packed)) bootblock;

//__attribute__((packed))

bootblock* boot_block;
inode_t* inode_start;
//need to by local?
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

int32_t read_exec_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t read_data_by_ptr(inode_t* inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t read_data_test(const uint8_t* fname, uint8_t* buf, uint32_t length);

int32_t dir_read(int32_t fd, uint8_t* buf, uint32_t nbytes);

int32_t file_read(int32_t fd, uint8_t* buf, uint32_t nbytes);

int32_t file_write(int32_t fd, const uint8_t* buf, uint32_t nbytes);

int32_t dir_write(int32_t fd, const uint8_t* buf, uint32_t nbytes);

int32_t file_open(const uint8_t* filename);

int32_t dir_open(const uint8_t* filename);

int32_t file_close(int32_t fd);

int32_t dir_close(int32_t fd);


#endif
