/* this file is the teminal driver use for put the cursor in the right place
	and print,delete the character ahead of the cursor. Also newline character
	as enter has been pressed */

#include "terminal.h"
#include "keyboard.h"
#include "lib.h"
#include "paging.h"
#include "syscall.h"

#define VIDEO_TM 0xB8000
#define VIDEO_4KB 0x1000
#define COLS 80
#define ROWS 25
#define UPDATE_MSK 0xFF
#define SHF_TO_HIGH 8
#define LINE_BUF_MAX 128
#define MOVE_UP_X 79
#define BOTTOM_Y_MAINTAIN 23
#define RESTORE_SCREEN_SIZE 1920
/*For page map when terminal switch*/
#define LARGE_PAGE_FLAG 0x87
#define V_BASE_ADDR		0x08000000//128MB
#define P_BASE_ADDR		0x00800000//8MB
#define LARGE_PAGE_PAD	0x00400000

#define TM_DEBUG 0

/* the vedio memory address use for terminal */
char* video_mem_tm = (char *)VIDEO_TM;
/* the video buffer for contain the current video memory */
char* video_tm_saver_0 = (char*) VIDEO_SAVE_0;
char* video_tm_saver_1 = (char*) VIDEO_SAVE_1;
char* video_tm_saver_2 = (char*) VIDEO_SAVE_2;
/* the x and y coordinate of the terminal */
int terminal_x;
int terminal_y;

int terminal_0_x;
int terminal_0_y;
int terminal_1_x;
int terminal_1_y;
int terminal_2_x;
int terminal_2_y;

term_t term[TM_MAX];
static uint8_t terminal_buf[3][128];

uint8_t font_color;
///////////////////////////////////////////////////////
/*	init_cursor
 *		Description: this function is use for initialize
 *					 the cursor position and put the cursor
 *					 at the left-top of the screen
 *		input: none
 *		output: none
 *		return: none
 *		side effect: change the VGA register in text mode
 */
 /////////////////////////////////////////////////////
void
init_cursor(void)
{
	terminal_x = 0;
	terminal_y = 0;
	//set low location register
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb(0x00,VGA_DATA_PORT);
 	//set the high location register
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb(0x00,VGA_DATA_PORT);
 	//font_color = ATTRIB_TM;
 }
///////////////////////////////////////////////////////
/*	update_cursor
 *		Description: update cursor position and print 
 *					 the character behind the blinking
 *					 cursor.
 * 		Input: uint8_t c -- the character that been print 
 *							to the screen
 *		Output: none
 *		Return: none
 * 		Side Effect: change the VGA register and video memory
 */
 /////////////////////////////////////////////////////
 void
 update_cursor(uint8_t c)
 {
 	if(terminal_y > ROWS ||terminal_x > COLS)
 	{
 		return;
 	}
 	//printf("here");
	if(terminal_y == ROWS)
	{
		vert_scroll_down();
		terminal_y = ROWS-1;
		terminal_x = 0;
		//maintaint the cursor position at the bottom
		unsigned short pos = (terminal_y*COLS) + terminal_x;
		outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
		outb((unsigned char)(pos&0xFF),VGA_DATA_PORT);
		outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
		outb((unsigned char)((pos>>SHF_TO_HIGH)&0xFF),VGA_DATA_PORT); 
	}
 	// put the character at right video memory address
 	*(uint8_t *)(video_mem_tm + ((COLS*terminal_y + terminal_x) << 1)) = c;
    *(uint8_t *)(video_mem_tm + ((COLS*terminal_y + terminal_x) << 1) + 1) = term[terminal_idx].font_color;
    //increment the terminal x-axis
    terminal_x++;
    //if reach the right screen, change to a new line
    if(terminal_x == COLS ) 
    {
    	terminal_x = 0;
    	terminal_y++;
    }
    //transfer the 2-D axis to 1-D index
    unsigned short pos = (terminal_y*COLS) + terminal_x;

    //update the cursor position
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb((unsigned char)(pos&UPDATE_MSK),VGA_DATA_PORT);
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb((unsigned char)((pos>>SHF_TO_HIGH)&UPDATE_MSK),VGA_DATA_PORT);

 	//for debug use
 	#if(TM_DEBUG == 1)
 	printf("%d,%d,%d ", terminal_x,terminal_y,term[terminal_idx].char_cnt);  
 	#endif
 }
//////////////////////////////////////////////////////
/*terminal_read
 *		Description: this function is being called as
 *					 the enter has been pressed. change
 *					 the cursor to the newline and print.
 *  		         store the command line into terminal buffer
 *					 and return
 *		Input: none
 *		Output: none
 *		Return: terminal_buf -- the terminal buffer to store the command line 
 *		Side Effect: change the VGA register 
 *
 */
 ////////////////////////////////////////////////////
void
newline_cursor(void)
{
	line_buf[terminal_idx][term[terminal_idx].char_cnt] = '\n';
	term[terminal_idx].char_cnt++;
	if(terminal_y == (ROWS-1))
	{
		vert_scroll_down();
	}
	//looping index for the line buffer
	int i; 
	//reset terminal x and y axis and character counter in line buffer
	terminal_x = -1;
	terminal_y ++;
	//transfer the 2-D axis to 1-D index
	unsigned short pos = (terminal_y*COLS) + terminal_x;
	//update the cursor
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb((unsigned char)(pos&UPDATE_MSK),VGA_DATA_PORT);
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb((unsigned char)((pos>>SHF_TO_HIGH)&UPDATE_MSK),VGA_DATA_PORT);
//	int32_t fd;
//	t_char_cnt = terminal_read(fd, terminal_buf, char_cnt);
	t_char_cnt = term[terminal_idx].char_cnt;
	for (i = 0; i < term[terminal_idx].char_cnt; i++)
		terminal_buf[terminal_idx][i] = line_buf[terminal_idx][i];

	for (i = 0; i<term[terminal_idx].char_cnt; i++)
		line_buf[terminal_idx][i] = 0x00;
	term[terminal_idx].char_cnt = 0;

	//copy the line buffer into terminal buffer and clear the line buffer
	//reset the character counter of the line buffer


	//for debug use
	#if(TM_DEBUG == 1)
 	printf("%d,%d,%d ", terminal_x,terminal_y,term[terminal_idx].char_cnt);  
	#endif
	
	return;
}
//////////////////////////////////////////////////////
/*backspace_cursor
 *	Description: this function is only being called
 *				 as the backspace has been pressed. it 
 *				 clear the previous character and move
 *				 the cursor backward
 *	Input: none
 *	Output: none
 * 	Return: none
 * 	Side Effect: change the video memory and VGA register
 */
 ////////////////////////////////////////////////////
void 
backspace_cursor(void)
{
	//first check is the line buffer is maximize
	if(term[terminal_idx].char_cnt == LINE_BUF_MAX)
		term[terminal_idx].char_cnt--;
	//if there is no character in the line buffer then this function do nothing
	if(term[terminal_idx].char_cnt == 0)
	{
		return;
	}
	//if the cursor is at the top-left corner, then do nothing
	if(terminal_x == 0 && terminal_y == 0)
	{
		return;
	}
	//get the current position of the cursor
	unsigned short pos = (terminal_y*COLS) + terminal_x;
	//move to the previous one
	pos = pos - 1;
	// update the previous video memory
	*(uint8_t *)(video_mem_tm + (pos << 1)) = 0x00;
    *(uint8_t *)(video_mem_tm + (pos << 1) + 1) = term[terminal_idx].font_color;
    // if the it is delete to the up rows, decrement y axis and reset x axis
    if(terminal_x == 0)
    {
    	terminal_x = MOVE_UP_X;
    	terminal_y--;
    }
    //else just decrement the x axis
    else
    {
    	terminal_x--;
    }
    //update the cursor position (move backward)
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb((unsigned char)(pos&0xFF),VGA_DATA_PORT);
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb((unsigned char)((pos>>SHF_TO_HIGH)&0xFF),VGA_DATA_PORT); 

 	//clear the line buffer as well
 	line_buf[terminal_idx][term[terminal_idx].char_cnt] = 0x00;
 	term[terminal_idx].char_cnt --;

 	#if(TM_DEBUG == 1)
 	printf("%d,%d,%d ", terminal_x,terminal_y,term[terminal_idx].char_cnt);  
 	#endif
}
///////////////////////////////////////////////////////
/*
 * vert_scroll_down 
 *			Description: this function is used for support vertical
 *						 scroll down as the character input from keyboard 
 *						 has reached the bottom line or the bottom right corner
 *						 the screen scroll down.
 * 			Input: none 
 * 			Output: none
 *			Return: none
 * 			Side Effect: modify the video memory
 */
 ///////////////////////////////////////////////////////
 void
 vert_scroll_down(void)
 {
	int i; //the looping index for video memory
	int j; //the traverse index for the restore screen
	j = COLS;
	uint8_t scroll_down_container[RESTORE_SCREEN_SIZE];
	for(i=0; i<RESTORE_SCREEN_SIZE; i++)
	{
		scroll_down_container[i] = *(uint8_t *)(video_mem_tm + (j << 1));
		j++;
	}
	
	for(i=0; i<RESTORE_SCREEN_SIZE; i++)
	{
		*(uint8_t *)(video_mem_tm + (i << 1)) = scroll_down_container[i];
		*(uint8_t *)(video_mem_tm + (i << 1) + 1) = term[terminal_idx].font_color;
	}
	for(i=RESTORE_SCREEN_SIZE; i<RESTORE_SCREEN_SIZE+COLS; i++)
	{
		*(uint8_t *)(video_mem_tm + (i << 1)) = 0x00;
		*(uint8_t *)(video_mem_tm + (i << 1) + 1) = term[terminal_idx].font_color;
	}
	terminal_y = BOTTOM_Y_MAINTAIN;
	terminal_x = 0;
	//maintaint the cursor position at the bottom
	unsigned short pos = (terminal_y*COLS) + terminal_x;
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb((unsigned char)(pos&0xFF),VGA_DATA_PORT);
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb((unsigned char)((pos>>SHF_TO_HIGH)&0xFF),VGA_DATA_PORT); 
 }
/////////////////////////////////////////////////////////////////
/*terminal_read
 * 		Description: This function is intend to read a buffer of character 
 *					 and return it into the terminal buffer for the further
 *					 use.
 *		Input: none
 * 		Output: none
 *		Return: the pointer to the stored buffer
 * 		Side effect: none
 */
//////////////////////////////////////////////////////////////////
int32_t 
terminal_read(int32_t fd, uint8_t* buf, uint32_t nbytes)
{
	if (buf == NULL)
		return -1;
	int i;	// the looping index
	int cnt = 0;
	
	//clear the terminal buffer first 
	for(i = 0; i<LINE_BUF_MAX || i< nbytes; i++)
		buf[i] = 0x00;

	//copy the buffer to the terminal buffer.
	sti();
	while (1) {
		if(t_char_cnt>0){
			for (i = 0; (i < nbytes || i < LINE_BUF_MAX) && i < t_char_cnt; i++)
			{
				buf[i] = terminal_buf[terminal_idx][i];
				cnt++;
			}
			if (buf[t_char_cnt -1] == '\n' || buf[t_char_cnt -1] == '\r')
				break;
		}
	}
	
	cli();
	for (i = 0; i<t_char_cnt; i++)
	{
		terminal_buf[terminal_idx][i] = 0x00;
	}
	t_char_cnt = 0;

	return (int32_t)i;
//	return -1;
}
/////////////////////////////////////////////////////////////////
/*terminal_write
 *		Description: this function is intend to write the strings on
 *				     to the terminal. and update the cursor and terminal_x 
 *					 and terminal y.
 *		Input: buf -- the buffer of characters that need to be write
 *   		   nbytes -- the number of byte that need to be printed to the screen
 *		Output: int32_t: -1 for failure, 0 for success
 *		Return: -1 or 0
 *		Side Effect: modifying the terminal_x and terminal_y, and changing the
 * 					 the VGA registers
 */
//////////////////////////////////////////////////////////////////
int32_t
terminal_write(int32_t fd, const uint8_t* buf, uint32_t nbytes)
{
	int i;
	int j;
	j = 0;
	int newline_flag = 0;
	if(buf == NULL)
		return -1;
	sti();
	for(i = 0; i<nbytes; i++)
	{
		//check if newline has been pressed
		if (buf[i] != 0){
			if(buf[i] == '\n' || buf[i] == '\r') {
				newline_flag = 1;
				terminal_y++;
				terminal_x=0;
				// if the terminal reach the 
				if(terminal_y == ROWS)
				{
					vert_scroll_down();
					terminal_y = ROWS-1;
					terminal_x = 0;
				}
			} else {
				// print the buffer to the screen
				*(uint8_t *)(video_mem_tm + ((COLS*terminal_y + terminal_x) << 1)) = buf[i];
				*(uint8_t *)(video_mem_tm + ((COLS*terminal_y + terminal_x) << 1) + 1) = term[terminal_idx].font_color;
				terminal_x++;
				//changing the line if it meets the COLS number
				if(terminal_x == COLS ) 
				{
					terminal_x = 0;
					terminal_y++;
				}
				//Scrolling down if countinuing printing stuff
				if(terminal_y == ROWS)
				{
					vert_scroll_down();
					terminal_y = ROWS-1;
					terminal_x = 0;
				}
			}
		}
	}
	unsigned short pos = (terminal_y*COLS) + terminal_x;
	//maintain and update the cursor position
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb((unsigned char)(pos&0xFF),VGA_DATA_PORT);
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb((unsigned char)((pos>>SHF_TO_HIGH)&0xFF),VGA_DATA_PORT); 

	
	//if the print is too long scrolling down the screen
	if(terminal_y == ROWS)
	{
		vert_scroll_down();
		terminal_y = ROWS-1;
		terminal_x = 0;
		//maintaint the cursor position at the bottom
		pos = (terminal_y*COLS) + terminal_x;
		outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
		outb((unsigned char)(pos&0xFF),VGA_DATA_PORT);
		outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
		outb((unsigned char)((pos>>SHF_TO_HIGH)&0xFF),VGA_DATA_PORT); 
	}
	cli();
	return 0;
}
//////////////////////////////////////////////////////////////////////
/* terminal open 
 *		Description: this function is intend to open the terminal driver 
 *				     and initialize it 
 *		Input: filename -- the file need to be opened
 *		Output: int32_t -- 0 for success
 *		Return: 0 for success open it
 *		Side Effect: clear the screen and initialize the cursor
 */
 /////////////////////////////////////////////////////////////////////
int32_t 
terminal_open(const uint8_t* filename)
{
	clear();
	init_cursor();
	return 0;
}
///////////////////////////////////////////////////////////////////////
/* terminal close
 *		Description: this function intend to close the terminal and further
 * 	 	Input: fd --
 * 		Output: int32_t -- 0 for successfully closed
 * 		Return: 0
 * 		Side Effect: none
 */
 //////////////////////////////////////////////////////////////////////
 int32_t
 terminal_close(int32_t fd)
 {
	 return 0;
 }

///////////////////////////////////////////////////////////////////////
/*terminal_switch
 *		Description: this function intends to switch terminal including 
 *					 flush TLB and copy the correct terminal's video memory
 *					 to the display video memeory space.
 *		Input: tm_idx -- the current activated terminal's number
 * 		Output: none
 *    	Retrun: none
 * 		Side effect: terminal is switching
 */
 //////////////////////////////////////////////////////////////////////
void 
terminal_switch(uint8_t tm_idx)
{
	// first check whether the current terminal is running
	// if it is running, restore the pid and flush the TLB
	if(term[tm_idx].running == 1)
	{
		pid_num = term[tm_idx].cur_pid;
		page_map(V_BASE_ADDR, P_BASE_ADDR + (pid_num - 1) * LARGE_PAGE_PAD, LARGE_PAGE_FLAG);
	}
	memcpy(video_mem_tm, (const void*)(term[tm_idx].vidmem_addr), VIDEO_4KB);
	//restore the cursor x and y position
	terminal_x = term[tm_idx].terminal_x;
	terminal_y = term[tm_idx].terminal_y;
	//update the current activated terminal index
	terminal_idx = tm_idx;

	unsigned short pos = (terminal_y*COLS) + terminal_x;
	//maintain and update the cursor position
	outb(CURSOR_LOCATION_L,VGA_IDX_PORT);
 	outb((unsigned char)(pos&0xFF),VGA_DATA_PORT);
 	outb(CURSOR_LOCATION_H,VGA_IDX_PORT);
 	outb((unsigned char)((pos>>SHF_TO_HIGH)&0xFF),VGA_DATA_PORT); 
 	return;

}
//////////////////////////////////////////////////////////////////
 /*init_all_video_buf
  *		Description: initialize all the three terminal's video memeory
  *   				 buffer. This function is only called once at initialization
  *					 of kernel.
  *		Input: none
  *		Output: none
  *		Return: none
  * 	Side Effect: clean all the three video memorry buffers
  */
 /////////////////////////////////////////////////////////////////

void
init_all_video_buf()
{
 	int32_t i,tm_idx;
	for (tm_idx = 0; tm_idx < 3; tm_idx++) {
		for(i=0; i<ROWS*COLS; i++) {
			*(term[tm_idx].vidmem_addr + (i << 1)) = ' ';
			*(term[tm_idx].vidmem_addr + (i << 1) + 1) = term[tm_idx].font_color;
		}
    }
}
/////////////////////////////////////////////////////////
/*save_current_terminal
 *		Description: save current activated terminal's screen.
 *
 *		Input: cur_tm -- the current activted terminal number
 *		Output: none
 *		Return: none
 * 		Side Effect: none
 */
 /////////////////////////////////////////////////////////
void 
save_current_terminal(uint8_t cur_tm)
{
	memcpy(term[cur_tm].vidmem_addr, (const void*)video_mem_tm, VIDEO_4KB);
	
	return;
}
/////////////////////////////////////////////////////////
/*save_current_cursor
 *		Description: save current activated terminal's cursor
 *					 position.
 *		Input: cur_tm -- the current activted terminal number
 *		Output: none
 *		Return: none
 * 		Side Effect: none
 */
 /////////////////////////////////////////////////////////
void
save_current_cursor(uint8_t cur_tm)
{
	term[cur_tm].terminal_x = terminal_x;
	term[cur_tm].terminal_y = terminal_y;
	
	return;
}


