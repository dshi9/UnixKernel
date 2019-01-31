/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
	//initialize the master's and slave's mask
	//blocking all the interrupt
	master_mask = 0xFF;
	slave_mask = 0xFF;
	outb(master_mask,PIC_MASTER_IMR);
	outb(slave_mask,PIC_SLAVE_IMR);
	//initialize the PIC
	outb(ICW1,MASTER_8259_PORT);
	outb(ICW1,SLAVE_8259_PORT);
	//ICW2 phase
	outb(ICW2_MASTER,MASTER_8259_SEC_PORT);
	outb(ICW2_SLAVE,SLAVE_8259_SEC_PORT);
	//ICW3 phase
	outb(ICW3_MASTER,MASTER_8259_SEC_PORT);
	outb(ICW3_SLAVE,SLAVE_8259_SEC_PORT);
	//ICW4 phase
	outb(ICW4,MASTER_8259_SEC_PORT);
	outb(ICW4,SLAVE_8259_SEC_PORT);
	printf("i8259 initialized\n");
}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
	// check the overall interrupt number
	if(irq_num > IRQ_TOTAL)
		return;

	// if the irq occurs on master
	if(irq_num < MASTER_IRQs)
	{
		uint8_t imr_mask = 0x01;
		//according to the interrupt index, build imr mask
		imr_mask = imr_mask << irq_num;
		//inverse the mask and store in the master, then write to the respect port
		master_mask = master_mask & (~imr_mask);
		outb(master_mask,MASTER_8259_SEC_PORT);
	}
	//if the irq occurs on slave
	else if( (irq_num > (MASTER_IRQs-1)) && (irq_num < (IRQ_TOTAL+1)))
	{
		uint8_t imr_mask = 0x01;
		//according to the interrupt index, build imr mask
		imr_mask = imr_mask << (irq_num-MASTER_IRQs);
		//inverse the mask and store in the slave, then write to the respect port
		slave_mask = slave_mask & (~imr_mask);
		enable_irq(PIC_CAS_IR);
		outb(slave_mask,SLAVE_8259_SEC_PORT);
	}

	return;

}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
	// check the overall interrupt number
	if(irq_num > IRQ_TOTAL)
		return;

	// if the irq occurs on master
	if(irq_num < MASTER_IRQs)
	{
		uint8_t imr_mask = 0x01;
		imr_mask = imr_mask << irq_num;
		//OR the mask with master and store in the master, then write to the respect port
		master_mask = master_mask | imr_mask;

		//write to master
		outb(master_mask,MASTER_8259_SEC_PORT);
	}
	// if the irq occurs on slave
	else if( (irq_num > (MASTER_IRQs-1)) && (irq_num < (IRQ_TOTAL+1)))
	{
		uint8_t imr_mask = 0x01;
		//according to the interrupt index, build imr mask
		imr_mask = imr_mask << (irq_num-MASTER_IRQs);
		//OR the mask with slave and store in the slave, then write to the respect port
		slave_mask = slave_mask | imr_mask;

		//if slave are all masked, turn off irq2
		if(slave_mask == 0xFF)
			disable_irq(PIC_CAS_IR);

		//write to slave
		outb(slave_mask,SLAVE_8259_SEC_PORT);
	}

	return;

}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
	if(irq_num & MASTER_IRQs)
	{
		// sending the eoi from irq which is greater than 8
		outb(EOI|(irq_num&(MASTER_IRQs-1)),SLAVE_8259_PORT);
		outb(EOI|PIC_CAS_IR,MASTER_8259_PORT);
	}
	else
	{
		// sending the eoi from irq which is lower than 8
		outb(EOI|irq_num,MASTER_8259_PORT);
	}
}

