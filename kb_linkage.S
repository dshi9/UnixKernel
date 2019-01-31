# this file is the asm wrapper for the keyboard
.global kb_linkage
#######################################
#
# kb_linkage:
#	description: keyboard interrupt handler's wrapper
#				 store and restore all the flags				  
#				 and registers. return to interrupt
# 	input:	none
#	output: none
# 	return:	none
# 	side_effect: return to the interrupt
#######################################
kb_linkage:
	#store all the registers and flags
	pushal
	pushfl
	#call the function
	call keyboard_handler
	#restore the  the registers and flags
	popfl
	popal
	# return to the interrupt
	iret
