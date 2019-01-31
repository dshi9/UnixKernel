/* rtc.c 
 *  This file use for rtc initilization, handlers and other functions involving
 *  comunication with the rtc chip.
 *  created by Dongwei Shi
 */


#include "rtc.h"
#include "lib.h"
#define rate_defalut 0x0F
#define IRQ_2 2 
#define B_ON 0x40
#define freq_2 0x0F
#define freq_4 0x0E
#define freq_8 0x0D
#define freq_16 0x0C
#define freq_32 0x0B
#define freq_64 0x0A
#define freq_128 0x09
#define freq_256 0x08
#define freq_512 0x07
#define freq_1024 0x06
#define HZ2 2
#define HZ4 4
#define HZ8 8
#define HZ16 16
#define HZ32 32
#define HZ64 64
#define HZ128 128
#define HZ256 256
#define HZ512 512
#define HZ1024 1024



unsigned char rate;

///////////////////////////////////////////////////////
/*
 * rtc_init:
 *	Description: This function uses for initilize the 
 *				 rtc chip to enable periodic interrupt. initilize the 
 *  rate of interrupt
 *
 *	input: none
 *  output: none
 *  return: none
 *	side effect: Set the the RTC chip to periodic interrupt
 *				 mode. RTC interrupt frequency is default.
 */
 //////////////////////////////////////////////////////
void
rtc_init(void)
{	
	cli();

	outb(REG_B,RTC_REG_IDX_PORT);//loacate register B
	char val=inb(RTC_DATA_PORT);//Get data from register B
	outb(REG_B,RTC_REG_IDX_PORT);//locate register B
	outb(val|B_ON,RTC_DATA_PORT);//turns on bit 6 of register B
	
	RTC_IR=0;
	outb(REG_A,RTC_REG_IDX_PORT);//loacate register A
	val=inb(RTC_DATA_PORT);//get value from register A
	outb(REG_A,RTC_REG_IDX_PORT);//loacate register A
	outb((val&RATE_MASK)|rate_defalut,RTC_DATA_PORT);  // set interrputs rate for RTC
	enable_irq(IRQ_2);
	enable_irq(RTC_IRQ); 
}
/////////////////////////////////////////////////////////
/*
 * rtc_handler:
 * 		Description: this function is being wrapped by 
 *					 assembly code. It is handle the interrupt
 *		input: none
 *		output: none
 *		return: none
 *      side effect: the screen is flickered with different symbols(cp1)
 */
 //////////////////////////////////////////////////////////
void
rtc_handler(void)
{
	cli();
	//test for check point 1
	//test_interrupts();

	//force to clear the rtc data port
	//in order to generate another interrupt
	outb(REG_C,RTC_REG_IDX_PORT);
	inb(RTC_DATA_PORT);
	RTC_IR=0;//set interrupt flag back to 0
	//end of interrupt
	send_eoi(RTC_IRQ);
	sti();
	return;
}
/////////////////////////////////////////////////////////
/*
* rtc_open
* Description: reinitialize rtc.
* input: filename 
* output: none
* return: 0
* side effect: returns 0
*/
/////////////////////////////////////////////////////////
int32_t rtc_open(const uint8_t* filename)
{
	rtc_init();
	return 0;
}

/////////////////////////////////////////////////////////
/*
* rtc_read
* Description: wait for interrupt
* input:fd,buf,nbytes.
* output: none
* return: 0 after the interrupt happens
* side effect: 
*/
/////////////////////////////////////////////////////////
int32_t rtc_read(int32_t fd, uint8_t* buf, uint32_t nbytes)
{
	cli();
	RTC_IR=1;
	sti();
	while(RTC_IR);
	return 0;
}



/////////////////////////////////////////////////////////
/*
* rtc_write
* Description: change the rtc interrupt frequency to a number which is power of 2, if the input 
frequency is not vliad, it falis 
* input:fd,buf,nbytes.
* output: none
* return: 0 for valid frequency, -1 for failures
* side effect: change the rtc interrupt frequency 
*/
/////////////////////////////////////////////////////////
int32_t rtc_write(int32_t fd, const uint8_t* buf, uint32_t nbytes)
{	if (nbytes != 4)
	return -1;

	//the char shift 24, 16 and 8 bits to construct a uint32_t
	uint32_t* freq = (uint32_t*)buf;
	switch (*freq)
	{
	case HZ2://change  the rate according to the buffer
	rate = freq_2;
	break;
	case HZ4:
	rate = freq_4;
	break;
	case HZ8:
	rate = freq_8;
	break;
	case HZ16:
	rate = freq_16;
	break;
	case HZ32:
	rate = freq_32;
	break;
	case HZ64:
	rate = freq_64;
	break;
	case HZ128:
	rate = freq_128;
	break;
	case HZ256:
	rate = freq_256;
	break;
	case HZ512:
	rate = freq_512;
	break;
	case HZ1024:
	rate = freq_1024;
	break;
	default:
	return -1;
	break;
	}
	cli();//
	outb(REG_A, RTC_REG_IDX_PORT); // write  frequency to rtc 
    char temp=inb(RTC_DATA_PORT); 
    outb(REG_A, RTC_REG_IDX_PORT); 
    outb((temp & RATE_MASK) | rate, RTC_DATA_PORT); //do OR opreation between old value and the setting rate to write new rate

	sti();
	return 0;
	}
	
/////////////////////////////////////////////////////////
/*
* rtc_close
* Description: close rtc.
* input: none
* output: none
* return: 0
* side effect: none
*/
/////////////////////////////////////////////////////////

int32_t rtc_close(int32_t fd)
{
	return 0;
}
