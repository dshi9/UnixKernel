/* this file intends to set up iterrupt tabel (idt) as initialize the kernel*/



#include "idt.h"
#include "types.h"
#include "x86_desc.h"
#include "exception.h"
#include "keyboard.h"
#include "rtc.h"
#include "lib.h"

#define KEYBOARD 0x21
#define RTC 0x28
#define SYS_CALL 0x80

////////////////////////////////////////////////////////////
/*
 * idt_init
 *		Description: this function is use for set up interrupt
 *					 table entry and only be called once in the 
 *					 initialization process.
 *
 *		input: none
 *		output: none
 *		return: none
 * 		side effect: none
 */
 ///////////////////////////////////////////////////////////
void 
idt_init(void)
{
	int i; //set up looping index

	/*set up exception data structure*/
	idt_desc_t exception;
	exception.seg_selector = KERNEL_CS;
	exception.reserved4 = 0x00;
	exception.reserved3 = 0;
	exception.reserved2 = 1;
	exception.reserved1 = 1;
	exception.size = 1;
	exception.reserved0 = 0;
	exception.dpl = 0;
	exception.present = 1;

	/* set up interrupt data structure */
	idt_desc_t interrupt;
	interrupt.seg_selector = KERNEL_CS;
	interrupt.reserved4 = 0x00;
	interrupt.reserved3 = 0;
	interrupt.reserved2 = 1;
	interrupt.reserved1 = 1;
	interrupt.size = 1;
	interrupt.reserved0 = 0;
	interrupt.dpl = 0;
	interrupt.present = 1;


	/* set up system_call register */
	idt_desc_t system_call;
	system_call.seg_selector = KERNEL_CS;
	system_call.reserved4 = 0x00;
	system_call.reserved3 = 0;
	system_call.reserved2 = 1;
	system_call.reserved1 = 1;
	system_call.size = 1;
	system_call.reserved0 = 0;
	system_call.dpl = 3;
	system_call.present = 1;


	// set up system call
	idt[SYS_CALL] = system_call;
	SET_IDT_ENTRY(idt[SYS_CALL],system_call_lnk);

	///////////////////////////////////////////////////////////////
	////////////////setting up the exception table/////////////////
	///////////////////////////////////////////////////////////////
	idt[0x00] = exception;													
	SET_IDT_ENTRY(idt[0x00], excep_divide_by_zero);				
	idt[0x01] = exception;										
	SET_IDT_ENTRY(idt[0x01], excep_debug);																				
	idt[0x02] = exception;										
	SET_IDT_ENTRY(idt[0x02], excep_NMI);						
	idt[0x03] = exception;
	SET_IDT_ENTRY(idt[0x03], excep_breakpoint);
	idt[0x04] = exception;
	SET_IDT_ENTRY(idt[0x04], excep_overflow);
	idt[0x05] = exception;
	SET_IDT_ENTRY(idt[0x05], excep_bound_range);
	idt[0x06] = exception;
	SET_IDT_ENTRY(idt[0x06], excep_invalid_opecode);
	idt[0x07] = exception;
	SET_IDT_ENTRY(idt[0x07], excep_device_not_available);
	idt[0x08] = exception;
	SET_IDT_ENTRY(idt[0x08], excep_double_fault);
	idt[0x09] = exception;
	SET_IDT_ENTRY(idt[0x09], excep_coprocessor_segment_overrun);
	idt[0x0A] = exception;
	SET_IDT_ENTRY(idt[0x0A], excep_invalid_tss);
	idt[0x0B] = exception;
	SET_IDT_ENTRY(idt[0x0B], excep_segment_not_present);
	idt[0x0C] = exception;
	SET_IDT_ENTRY(idt[0x0C], excep_stack_seg_fault);
	idt[0x0D] = exception;
	SET_IDT_ENTRY(idt[0x0D], excep_general_protection);
	idt[0x0E] = exception;
	SET_IDT_ENTRY(idt[0x0E], excep_page_fault);
	idt[0x10] = exception;
	SET_IDT_ENTRY(idt[0x10], excep_floating_point);
	idt[0x11] = exception;
	SET_IDT_ENTRY(idt[0x11], excep_alin_check);
	idt[0x12] = exception;
	SET_IDT_ENTRY(idt[0x12], excep_machine_check);
	idt[0x13] = exception;
	SET_IDT_ENTRY(idt[0x13], excep_simd_floating_point);
	idt[0x14] = exception;
	SET_IDT_ENTRY(idt[0x14], excep_virtualization);
	idt[0x1E] = exception;
	SET_IDT_ENTRY(idt[0x1E], excep_security);
	///////////////////////////////////////////////////////////////////
	////////////////exception table set up ends here///////////////////
	///////////////////////////////////////////////////////////////////

	//set upt interrupt for idt
	for (i = 0x20; i < 0x30; i++)
	{
		idt[i] = interrupt;
	}

	/* set up the keyboard */
	SET_IDT_ENTRY(idt[KEYBOARD],kb_linkage);

	/* set up the RTC */
	SET_IDT_ENTRY(idt[RTC],rtc_linkage);


	/* don't forgot load the idtr !!!!*/
	lidt(idt_desc_ptr);
}

