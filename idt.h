#ifndef IDT_H
#define IDT_H
 
extern void kb_linkage(void);
extern void rtc_linkage(void);
extern void system_call_lnk(void);
void idt_init(void);
#endif



