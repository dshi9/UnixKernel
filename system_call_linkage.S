#this is system call assembly wrapper 
#use for return to the user space after system call is being called

.global system_call_lnk


.data
	NR_syscalls_end = 10
	NR_syscalls_start = 1
	U_CS = 0x0023
	U_DS = 0x002B
#######################################
#
# system_call_linkage:
#	description: system call assemnbly wrapper
#				 store and restore all the flags				  
#				 and registers. return to interrupt
# 	input:	none
#	output: none
# 	return:	none
# 	side_effect: switch back to user mode
#######################################
system_call_lnk:
	#save all register
	pushl %esp
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushfl

	

	#check the current system call number
	#make sure there is no other invalid system call
	cmpl $NR_syscalls_start,%eax
	jl badsys
	cmpl $NR_syscalls_end,%eax
	jge badsys
	

	#system call is being called
	#On x86, the registers ebx, ecx, edx, esi, and edi contain, in order, the first five arguments
	pushl %edx #3rd Arg
	pushl %ecx #2nd Arg
	pushl %ebx #1st Arg
	call *sys_call_table(,%eax,4)
	#tear down the stack by 12 since 3 arguments
	addl $12,%esp
	jmp resume_user_space
badsys:
#invalid system call, return -1;
	movl $-1,%eax
	#resume to the user space
resume_user_space:
	#restore the flags and registers that saved before
	popfl
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popl %esp
	iret

######################### System call jump table ##########################
sys_call_table:
	.long 0x0
	.long halt
	.long execute
	.long read
	.long write
	.long open
	.long close	
	.long getargs
	.long vidmap
	.long set_handler
	.long sigreturn
########################################################################




