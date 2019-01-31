/* exception.c is the handlers of excpetion, mapping in the idt, calling as an interrupt*/
#include "exception.h"
#include "lib.h"

/* divide by zero exception handler*/
void excep_divide_by_zero(void)
{
	printf("divide by zero exception\n");
	while(1);
}
/* debug exception handler*/
void excep_debug(void)
{
	printf("debug exception\n");
	while(1);
}
/* NMI interrupt handler */
void excep_NMI(void)
{
	printf("NMI interrupt exception\n");
	while(1);
}
/* breakpoint handler */
void excep_breakpoint(void)
{
	printf("breakpoint exception\n");
	while(1);
}
/*overflow handler*/
void excep_overflow(void)
{
	printf("overflow exception\n");
	while(1);
}
/* BOUND range handler */
void excep_bound_range(void)
{
	printf("BOUND range exceeded exception\n");
	while(1);
}
/* invalid opcode handler */
void excep_invalid_opecode(void)
{
	printf("invalid opcode exception\n");
	while(1);
}
/* device not available handler */
void excep_device_not_available(void)
{
	printf("device not available exception\n");
	while(1);
}
/* double fault handler */
void excep_double_fault(void)
{
	printf("double fault exception\n");
	while(1);
}
/* coprocessor segment overrun handler */
void excep_coprocessor_segment_overrun(void)
{
	printf("coprocessor segment overrun exception\n");
	while(1);
}
/* invalid tss handler */
void excep_invalid_tss(void)
{
	printf("invalid task state segment exception\n");
	while(1);
}
/* segment not present handler */
void excep_segment_not_present(void)
{
	printf("segment not present exception\n");
	while(1);
}
/* stack fault segment handler */
void excep_stack_seg_fault(void)
{
	printf("stack segmentation fault exception\n");
	while(1);
}
/* general protect fault  handler */
void excep_general_protection(void)
{
	printf("general protection fault exception\n");
	while(1);
}
/* page fault handler */
void excep_page_fault(void)
{
	printf("page fault exception\n");
	while(1);
}
/* floating point handler */
void excep_floating_point(void)
{
	printf("x87 floating point exception\n");
	while(1);
}
/* alignment check handler*/
void excep_alin_check(void)
{
	printf("alignment check exception\n");
	while(1);
}
/* machine check handler */
void excep_machine_check(void)
{
	printf("machine check exception\n");
	while(1);
}
/* simd floating handler */
void excep_simd_floating_point(void)
{
	printf("simd floating point exception\n");
	while(1);
}
/*virtualization handler */
void excep_virtualization(void)
{
	printf("virtualization exception\n");
	while(1);
}
/* securiy handler*/
void excep_security(void)
{
	printf("security excpetion\n");
	while(1);
}




