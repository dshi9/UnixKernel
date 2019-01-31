#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "terminal.h"

extern uint8_t line_buf[3][128];
extern int char_cnt;
extern int t_char_cnt;
extern uint8_t terminal_idx;
/* initialization of keyboard */
void keyboard_init(void);
/* keyboard handler */
void keyboard_handler(void);
#endif /* _KEYBOARD_H */




