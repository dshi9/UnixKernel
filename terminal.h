#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

#define VGA_IDX_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5
#define CURSOR_LOCATION_H 0x0E
#define CURSOR_LOCATION_L 0x0F

/*for terminal*/
#define TM_MAX		3
#define ATTRIB_TM 0x7
#define ATTRIB_TM_1 0x9
#define ATTRIB_TM_2 0x3 
#define VIDEO_SAVE_0 0x1000
#define VIDEO_SAVE_1 0x3000
#define VIDEO_SAVE_2 0x5000


extern int terminal_x;
extern int terminal_y;

/*structure for terminal*/
typedef struct term_t
{
	uint32_t	esp;
	uint32_t	ebp;
	uint32_t	term_pid;
	uint32_t	cur_pid;
	uint32_t	process_num;
	uint32_t	running;
	int			terminal_x;
	int			terminal_y;
	char*		vidmem_addr;
	uint8_t		font_color;
	int 		char_cnt;

} term_t;

extern term_t term[TM_MAX];
/*updating cursor */
void init_cursor(void);
void update_cursor(uint8_t c);
void newline_cursor(void);
void backspace_cursor(void);
void vert_scroll_down(void);
/* switch terminal helper function */
void terminal_switch(uint8_t tm_idx);
void init_all_video_buf(void);
void save_current_terminal(uint8_t cur_tm);
void save_current_cursor(uint8_t cur_tm);
//system call for the terminal driver
int32_t terminal_read(int32_t fd, uint8_t* buf, uint32_t nbytes);
int32_t terminal_write(int32_t fd, const uint8_t* buf, uint32_t nbytes);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);

#endif  /* _TERMINAL_H */



