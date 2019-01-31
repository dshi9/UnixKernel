/* keyboard.c 
 *  This file use for keyboard initilization, handlers and other functions involving
 *  comunication with the keyboard.
 *  created by Dongwei Shi
 */
#include "keyboard.h"
#include "lib.h"
#include "syscall.h"
#include "x86_desc.h"

/* the port to connect with keyboard */
#define KEYBOARD_PORT 0x60
#define KEYBOARD_IRQ	1
#define RELEASE_OFFSET  0x80
#define LEFT_SHF 0x36
#define RIGHT_SHF 0x2A
#define LEFT_SHF_R 0xB6
#define RIGHT_SHF_R 0xAA
#define CAPSLOCK 0x3A
#define ENTR	0x1C
#define LINE_BUF_SIZE 128
#define BK_SPACE 0x0E
#define CTRL 0x1D
#define L_BUTTON			0x26
#define CTRL_R 0x9D
#define ALT 0x38
#define ALT_R 0xB8
#define TAB 0xF
#define F_1 0x3B
#define F_2 0x3C
#define F_3 0x3D
#define F_4 0x3E

/*for terminal*/
#define ATTRIB_TM 0x7
#define ATTRIB_TM_1 0x9
#define ATTRIB_TM_2 0x3 
#define VIDEO_SAVE_0 0x1000
#define VIDEO_SAVE_1 0x3000
#define VIDEO_SAVE_2 0x5000

/* the look up table for the keyboard */
unsigned char code[128] =
{
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '[', ']',
	0, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
	0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
	0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/* shift version keyboard */
unsigned char code_shift[128] = 
{
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '{', '}',
	0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0,'|','Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
	0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
	0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/* capslock version keyboard */
unsigned char code_cap[128] = 
{
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	0, 0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '[', ']',
	0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
	0,'\\','Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*',
	0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
	0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/* capslock keyboard with shifts on */
unsigned char code_cap_shift[128] = 
{
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '{', '}',
	0, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '"', '`',
	0,'|','z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, '*',
	0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
	0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/* line buffer */
uint8_t line_buf[3][128];
int t_char_cnt = 0;
int line_buf_i;
/* shift flag */
static uint8_t shift_is_pressed = 0;
/* capslock flag */
static uint8_t cap_flag = 0;
uint8_t ctrl_flag;
uint8_t alt_flag;
uint8_t ret;
uint8_t terminal_idx;
uint8_t tm2_first_time;
uint8_t tm3_first_time;

uint32_t tm_ebp[TM_MAX];
uint32_t tm_esp[TM_MAX];
///////////////////////////////////////////////////
/*
 * keyboard_init:
 *		Description: this is function intend to initialize
 *					 keyboard. just simply turn on the keyboard
 * 					 interrupt on PIC
 *		input: none
 *		output: none
 *		return: none
 *      side effect: keyboard is being interrupt-on mode
 */
 /////////////////////////////////////////////////

void
keyboard_init(void)
{
	//enable the keyboard interrupt
	enable_irq(KEYBOARD_IRQ);
	//first terminal is setting up
	terminal_idx = 0;
}
///////////////////////////////////////////////////
/*
 * keyboard_handler:
 *		Description: this is function intend to handle the data
 *					 from keyboard. echo the keycode(cp1)
 *		input: none
 *		output: none
 *		return: none
 *      side effect: the echoed keycode is being printed on console
 */
 /////////////////////////////////////////////////
void
keyboard_handler(void)
{
	cli();
	uint32_t scancode;
	//read the value from the keyborad port 0x70
	scancode = inb(KEYBOARD_PORT);
	//check for the shift has been pressed or not
	if(scancode == LEFT_SHF || scancode == RIGHT_SHF)
	{
		shift_is_pressed = 1;
	}
	if(scancode == LEFT_SHF_R || scancode == RIGHT_SHF_R)
	{
		shift_is_pressed = 0;
	}
	
	//check for the ctrl is been pressed or released
	if(scancode == CTRL)
	{
		ctrl_flag = 1;
	}
	if(scancode == CTRL_R)
	{
		ctrl_flag = 0;
	}
	//printf("%x",scancode);
	//check for atl is been pressed or released
	if(scancode == ALT)
	{
		alt_flag = 1;
	}
	if(scancode == ALT_R)
	{
		alt_flag = 0;
	}
	//look up the correct character
	if ((scancode & RELEASE_OFFSET) == 0) {
		//use for look at scan code
		//printf("%x",scancode);
		//invalid input
		if(scancode == TAB || scancode == CTRL || scancode == ALT || scancode == CTRL_R || scancode == ALT_R || scancode == F_4){
			send_eoi(KEYBOARD_IRQ);
			sti();
			return;
		}
		//enter has been pressed
		if( scancode == ENTR)
		{
			newline_cursor();
		}
		//backspace has been pressed
		if(scancode == BK_SPACE)
		{
			backspace_cursor();
		}
		// saving current terminal and cursor
		save_current_terminal(terminal_idx);
		save_current_cursor(terminal_idx);
		term[terminal_idx].cur_pid = pid_num;
		asm volatile ("movl %%esp, %0":"=g"(term[terminal_idx].esp));
		asm volatile ("movl %%ebp, %0":"=g"(term[terminal_idx].ebp));
		// terminal switcher
		if(alt_flag == 1 && scancode>=F_1 && scancode <=F_3)
		{
			terminal_idx = scancode-F_1;
			terminal_switch(terminal_idx);
			/*run shell if this is first time*/
			if (term[terminal_idx].running == 0)
			{
				send_eoi(KEYBOARD_IRQ);
				sti();
				execute((uint8_t*)"shell");
			}
			tss.esp0 = 0x00800000 - (0x2000 *pid_num) - 4;
			tss.ss0 = KERNEL_DS;

			send_eoi(KEYBOARD_IRQ);
			asm volatile (
				"MOVL %0, %%ebp;"
				"MOVL %1, %%esp;"
				:
			: "r"(term[terminal_idx].ebp), "r"(term[terminal_idx].esp)
				: "esp", "ebp", "memory", "cc" //clobbered register 
				);
			//alt_flag = 0;
			return;
		}
		//printf("TM:%d\n",terminal_idx);
		//CapsLock ha\s been pressed
		if(scancode == CAPSLOCK)
		{
			//if capslock botton has been pressed and released toggle cap_flag
			if(cap_flag)
			{
				cap_flag = 0;
			}
			else
			{
				cap_flag = 1;
			}
		}

		if(!cap_flag)
		{
			//look up different keycode
			if(shift_is_pressed)
				ret = code_shift[scancode];

			else
 				ret = code[scancode];

 			//if shift has been pressed don't print anything 
 			if(term[terminal_idx].char_cnt < LINE_BUF_SIZE){
 				if((scancode != RIGHT_SHF) && (scancode != LEFT_SHF) && (scancode != CAPSLOCK) && (scancode != ENTR) && (scancode != BK_SPACE) && (scancode != F_1) && (scancode != F_2) && (scancode != F_3))
 				{
					line_buf[terminal_idx][term[terminal_idx].char_cnt] = ret;
					//increment the line buffer
					term[terminal_idx].char_cnt ++;
				}
			}
		}
		else
		{
			//look up different keycode
			if(shift_is_pressed)
				ret = code_cap_shift[scancode];

			else
 				ret = code_cap[scancode];

 			//if shift has been pressed don't print anything 
 			if(term[terminal_idx].char_cnt < LINE_BUF_SIZE){
 				if((scancode != RIGHT_SHF) && (scancode != LEFT_SHF) && (scancode != CAPSLOCK) && (scancode != ENTR) && (scancode != BK_SPACE) && (scancode != F_1) && (scancode != F_2) && (scancode != F_3))
 				{
					line_buf[terminal_idx][term[terminal_idx].char_cnt] = ret;
					//increment the line buffer
					term[terminal_idx].char_cnt++;
				}
			}
		}
		//if line buffer is full, don't update the cursor
		if(term[terminal_idx].char_cnt <LINE_BUF_SIZE && (scancode != BK_SPACE) && (scancode != CAPSLOCK) && (scancode != RIGHT_SHF) && (scancode != LEFT_SHF) && (scancode != F_1) && (scancode != F_2) && (scancode != F_3))
			update_cursor(line_buf[terminal_idx][term[terminal_idx].char_cnt-1]);


		//if Ctrl+L has been pressed, clear the screen
		if((ctrl_flag == 1) && (scancode == L_BUTTON)){
			clear();
			//initialzie the cursor position
		    init_cursor();
			//clear the line buffer
			for (line_buf_i = 0; line_buf_i<term[terminal_idx].char_cnt; line_buf_i++)
			{
				line_buf[terminal_idx][line_buf_i] = 0x00;
			}
			//reset the character counter to 0 
			term[terminal_idx].char_cnt = 0;
		}
 	}

	send_eoi(KEYBOARD_IRQ);
	sti();
	return;
}
