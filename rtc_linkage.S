#this is the rtc assembly wrapper in order to return to interrupt
.global rtc_linkage

#######################################
#
# rtc_linkage:
#	description: rtc interrupt handler's wrapper
#				 store and restore all the flags				  
#				 and registers. return to interrupt
# 	input:	none
#	output: none
# 	return:	none
# 	side_effect: return to the interrupt
#######################################
rtc_linkage:
	#store all the registers and flags
	pushal
	pushfl
	#call the function
	call rtc_handler
	#restore all the registers and flags
	popfl
	popal
	#return to the interrupt
	iret

	

