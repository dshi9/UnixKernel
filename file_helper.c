#include "file_helper.h"
#include "lib.h"
#include "terminal.h"
#include "syscall.h"

#define block_pad		4096
#define block_pad_bit	12
#define fname_length	32
#define P_BASE_ADDR		0x00800000//8MB
#define PCB_PAD			0x2000//8kb
uint32_t dir_index = 0;
uint32_t dir_itself_read = 0;

//////////////////////////////////////////////////////////////
/*read_dentry_by_name
*   DESCRIPTION: Read directory entry by filename
*   INPUTS: fname  -- the filename of which dir entry we will read
*			dentry -- we will place the dir entry here when we find it
*   OUTPUTS: none
*   RETURN VALUE: -1  -- non-existent file
*				   0  -- we found matched file name
*   SIDE EFFECTS: change dentry if we found matched file name
*/
///////////////////////////////////////////////////////////////
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
	int i;
//	int j = 0;
	if (fname == NULL)
		return -1;//invalid string pointer
	dentry_t* test = &(boot_block->dir_itself);
	/*compare 32 bytes file name*/
	if (strncmp((int8_t*)(test->filename), (int8_t*)(fname), fname_length) == 0)
	{
		*dentry = *test;
		return 0;
	}
	for (i = 0; i < boot_block->num_dir; i++) {
		dentry_t* test = &(boot_block->dir_entry[i]);
		/*
		//we test if filename matches until last char or we reach end of file name and 32 is max char of filename
		while ((j < 32) && (fname[j] != 0 )&& (test->filename[j] != 0)) {
			if (test->filename[j] != fname[j]) {
				break;//chekc next filename since there is at least one difference
			}
			else {
				//if we are check last char of filename or when the filename ends, we find the file
				//31 is the end of filename
				if (j == 31) {
					*dentry = *test;
					return 0; //return 0 if we find file matched fname
				}
				else if(((test->filename[j + 1] == 0 && fname[j + 1] == 0)) || ((test->filename[j + 1] == '\n' && fname[j + 1] == '\n'))){
					*dentry = *test;
					return 0; //return 0 if we find file matched fname
				}
			}
			j++;
		}
		j = 0;
		*/
		/*compare 32 bytes file name*/
		if(strncmp((int8_t*)(test->filename), (int8_t*)(fname), fname_length)==0)
		{
			*dentry = *test;
			return 0;
		}
		
	}
	
	/*return -1 if non-existent file*/
	return -1;
}

///////////////////////////////////////////////////////////////
/*read_dentry_by_name
*   DESCRIPTION: Read directory entry by filename
*   INPUTS: fname  -- the filename of which dir entry we will read
*			dentry -- we will place the dir entry here when we find it
*   OUTPUTS: none
*   RETURN VALUE: -1  -- non-existent file
*				   0  -- we found matched file name
*   SIDE EFFECTS: change dentry if we found matched file name
*/
///////////////////////////////////////////////////////////////
int32_t read_exec_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
	int i;
	//	int j = 0;
	if (fname == NULL)
		return -1;//invalid string pointer
	for (i = 0; i < boot_block->num_dir; i++) {
		dentry_t* test = &(boot_block->dir_entry[i]);
		/*compare 32 bytes file name*/
		if (strncmp((int8_t*)(test->filename), (int8_t*)(fname), fname_length) == 0)
		{
			*dentry = *test;
			return 0;
		}

	}

	/*return -1 if non-existent file*/
	return -1;
}

///////////////////////////////////////////////////////////////
/* read_dentry_by_name
*   DESCRIPTION: Read directory entry by index
*   INPUTS: index  -- the index of which dir entry we will read
*			dentry -- we will place the dir entry here when we find it
*   OUTPUTS: none
*   RETURN VALUE: -1  -- invalid index or we did not find such index 
*				   0  -- we found matched file name
*   SIDE EFFECTS: change dentry if we found matched file name
*/
///////////////////////////////////////////////////////////////
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
	if (index < boot_block->num_dir && index < 62) { //62 is the max num of inode
		*dentry = boot_block->dir_entry[index];
		return 0;
	}

	/*return -1 if invalid index*/
	return -1;
}

///////////////////////////////////////////////////////////////
/* read_data
*   DESCRIPTION: Word much like read system call, reading up to length bytes 
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: inode  -- the inode where we are looking for data
*			offset -- offset position in inode
*			buf	   -- the btyes read stored here
*			length -- length from offset position in inode
*   OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: -1	-- invalid inode passed in
*				  other -- the bytes read and is placed in buf
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
	int32_t length_read = 0;
	uint32_t data_block_index, data_index;
	uint8_t* data_to_read;
//	uint32_t i;
	if (inode < boot_block->num_inode) {
		inode_t* inode_read = (inode_t*)(inode_start + inode);
		if (offset < (inode_read->length)) {
			data_block_index = offset >> (block_pad_bit);//this is for data index inside inode
			if (inode_read->block_num[data_block_index] >= boot_block->num_data)
				return -1; //return -1 if invalid data block num found
			data_index = offset % (block_pad);
			data_to_read = (uint8_t*)(inode_t*)(inode_start + (boot_block->num_inode + inode_read->block_num[data_block_index]));
			data_to_read = (uint8_t*)(data_to_read + data_index);//we access the data we need to read
//			printf("%c", *data_to_read);
			/*data index should be less than 1023*/
			while (length_read < length && ((offset + length_read) < (inode_read->length)) && data_block_index<1023) {
				data_to_read = (uint8_t*)(inode_t*)(inode_start + (boot_block->num_inode + inode_read->block_num[data_block_index]));
				data_to_read = (uint8_t*)(data_to_read + data_index);
				buf[length_read] = *data_to_read;
				length_read++;
				//if we have not reach de end of block, we will simply increment index
				data_index++;
				if (data_index < block_pad) {
					data_to_read++;
				}
				//if we have reach the end of block, we copy data from next block index
				else {
					data_index = 0;
					data_block_index++;
					if (inode_read->block_num[data_block_index] >= boot_block->num_data)
						return -1; //return -1 if invalid data block num found
				}
			}
		}
		else
			return 0; // reach the end of file
	}

	/*return number read*/
	return length_read;
}

///////////////////////////////////////////////////////////////
/* read_data_by_ptr
*   DESCRIPTION: Word much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: inode  -- the inode where we are looking for data
*			offset -- offset position in inode
*			buf	   -- the btyes read stored here
*			length -- length from offset position in inode
*   OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: -1	-- invalid inode passed in
*				  other -- the bytes read and is placed in buf
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t read_data_by_ptr(inode_t* inode, uint32_t offset, uint8_t* buf, uint32_t length) {
	int32_t length_read = 0;
	uint32_t data_block_index, data_index;
	uint8_t* data_to_read;
	//	uint32_t i;
	if (inode != NULL) {
		inode_t* inode_read = inode;
		if (offset < (inode_read->length)) {
			data_block_index = offset >> (block_pad_bit);//this is for data index inside inode
			if (inode_read->block_num[data_block_index] >= boot_block->num_data)
				return -1; //return -1 if invalid data block num found
			data_index = offset % (block_pad);
			data_to_read = (uint8_t*)(inode_t*)(inode_start + (boot_block->num_inode + inode_read->block_num[data_block_index]));
			data_to_read = (uint8_t*)(data_to_read + data_index);//we access the data we need to read
																 //			printf("%c", *data_to_read);
			while (length_read < length && (offset + length_read) < (inode_read->length)) {
				data_to_read = (uint8_t*)(inode_t*)(inode_start + (boot_block->num_inode + inode_read->block_num[data_block_index]));
				data_to_read = (uint8_t*)(data_to_read + data_index);
				buf[length_read] = *data_to_read;
				length_read++;
				//if we have not reach de end of block, we will simply increment index
				data_index++;
				if (data_index < block_pad) {
					data_to_read++;
				}
				//if we have reach the end of block, we copy data from next block index
				else {
					data_index = 0;
					data_block_index++;
					if (inode_read->block_num[data_block_index] >= boot_block->num_data)
						return -1; //return -1 if invalid data block num found
				}
			}
		}
		else
			return 0; // reach the end of file
	}

	/*return -1 if invalid inode_num passed in*/
	return length_read;
}

///////////////////////////////////////////////////////////////
/* read_data_test
*   DESCRIPTION: Word much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fname  -- the filename of which dir entry we will read
*			buf	   -- the btyes read stored here
*			length -- length from offset position in inode
*   OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: -1	-- invalid inode passed in
*				  other -- the bytes read and is placed in buf
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t read_data_test(const uint8_t* fname, uint8_t* buf, uint32_t length) {
	dentry_t* dentry;
	int test_read;
	test_read = read_dentry_by_name(fname, dentry);
	
	/*test if we find such dir entry with matched file name*/
	if (test_read == -1)
		return -1;//indicate non-existent file
	
	test_read = read_data(dentry->inode_num, 0, buf, length);//we read from the start of file

	return test_read;
}

///////////////////////////////////////////////////////////////
/* dir_read
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fd  -- the filename of which dir entry we will read
*			buf	   -- the btyes read stored here
*			nbytes -- length from offset position in inode
*   OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: 0 -- dir_read finish
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t dir_read(int32_t fd, uint8_t* buf, uint32_t nbytes) {
//	uint32_t dir_index = 0;
	if (buf == NULL)
		return -1;
	int i = 0;
	int temp_val;
	/*test if we read dir itself*/
	if (dir_itself_read == 0){
		/*compare 32 bytes file name*/
		while (i < nbytes && i < fname_length) {
			buf[i] = boot_block->dir_itself.filename[i];
			i++;
		}
		dir_itself_read = 1;
		return i;
	}

	/*read remaining stuffs in file system*/
	if (dir_index < boot_block->num_dir) {
		//dentry_temp will return dir entry we read
		dentry_t dentry_temp;
		temp_val = read_dentry_by_index(dir_index, &dentry_temp);
		/*if we read entry successfully, we copy to buf*/
		if(temp_val == 0){
			while (i < nbytes && i < fname_length) {
				buf[i] = boot_block->dir_entry[dir_index].filename[i];
				i++;
			}
		}
//		i = 0;
		dir_index++;
	}
	if (dir_index == boot_block->num_dir){
		dir_index = 0;
		dir_itself_read = 0;
		return 0;
	}
	return i;
}

///////////////////////////////////////////////////////////////
/*  file_read
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fd  -- the filename of which dir entry we will read
*			buf	   -- the btyes read stored here
*			nbytes -- length from offset position in inode
*	OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: -1	-- invalid inode passed in
*				  other -- the bytes read and is placed in buf
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t file_read(int32_t fd, uint8_t* buf, uint32_t nbytes) {

	int read_bytes;

	/*get current pcblock*/
	uint32_t pid_number = get_pid();
	pcb_t* pcblock = (pcb_t*)(P_BASE_ADDR - pid_number*PCB_PAD);

	uint32_t file_pos = pcblock->fd_array[fd].fpos;

	/*test if we find such dir entry with matched file name
	if (test_read == -1)
		return -1;//indicate non-existent file
	*/
	read_bytes = read_data_by_ptr(pcblock->fd_array[fd].inode_ptr, file_pos, buf, nbytes);//we read from the start of file
	pcblock->fd_array[fd].fpos += read_bytes;
	return read_bytes;
//	return -1;//?
}

///////////////////////////////////////////////////////////////
/*  file_write
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fd  -- the filename of which dir entry we will read
*			buf	   -- the btyes read stored here
*			nbytes -- length from offset position in inode
*   OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: -1	-- failure to write
*				  other -- the bytes read and is placed in buf
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t file_write(int32_t fd, const uint8_t* buf, uint32_t nbytes) {
	return -1;
}

///////////////////////////////////////////////////////////////
/*  dir_write
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fd  -- the filename of which dir entry we will read
*			buf	   -- the btyes read stored here
*			nbytes -- length from offset position in inode
*   OUTPUTS: bytes read will be placed in buffer
*   RETURN VALUE: -1	-- failure to write
*				  other -- the bytes read and is placed in buf
*   SIDE EFFECTS: change buf
*/
///////////////////////////////////////////////////////////////
int32_t dir_write(int32_t fd, const uint8_t* buf, uint32_t nbytes) {
	return -1;
}

///////////////////////////////////////////////////////////////
/*  file_open
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: filename -- the filename to open
*   OUTPUTS: none
*   RETURN VALUE: -1	-- invalid inode passed in
*				  0		-- open successfully
*   SIDE EFFECTS: none
*/
///////////////////////////////////////////////////////////////
int32_t file_open(const uint8_t* filename) {

		return 0;
}

///////////////////////////////////////////////////////////////
/*  dir_open
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: filename -- the filename to open
*   OUTPUTS: none
*   RETURN VALUE: -1	-- invalid inode passed in
*				  0		-- open successfully
*   SIDE EFFECTS: none
*/
///////////////////////////////////////////////////////////////
int32_t dir_open(const uint8_t* filename) {
	return 0;
}

///////////////////////////////////////////////////////////////
/*  file_close
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fd -- the filename to open
*   OUTPUTS: none
*   RETURN VALUE: -1	-- invalid inode passed in
*				  0		-- open successfully
*   SIDE EFFECTS: none
*/
///////////////////////////////////////////////////////////////
int32_t file_close(int32_t fd) {
	return 0;
}

///////////////////////////////////////////////////////////////
/*  dir_close
*   DESCRIPTION: Work much like read system call, reading up to length bytes
*				 starting from position offset in the file with inode number inode
*				 returning the number of tyes read and plcaed in the buffer
*   INPUTS: fd -- the filename to open
*   OUTPUTS: none
*   RETURN VALUE: -1	-- invalid inode passed in
*				  0		-- open successfully
*   SIDE EFFECTS: none
*/
///////////////////////////////////////////////////////////////
int32_t dir_close(int32_t fd) {
	return 0;
}

