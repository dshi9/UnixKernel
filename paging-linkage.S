# this file is use for enable paging and external paging


.global	enablePaging
.global	enableExPaging

#
# enablePaging
#   DESCRIPTION: This function will enable paging by set 32th bit of CR0 to 1
#   INPUTS: none
#   OUTPUTS: none
#   RETURN VALUE: none
#   SIDE EFFECTS: Change the 32th bit of CR0
enablePaging:
#stack setup
pushl	%ebp
movl 	%esp, %ebp
#load cr0 and enable 32th bit to one to enable paging 
movl	%cr0, %eax
#0x80000000 has 1 in the most significant bit
orl	$0x80000000,%eax
movl	%eax,%cr0
#stack teardown
leave
ret

#
# enablePaging
#   DESCRIPTION: This function will enable extended paged by set 5th bit of CR4 to 1
#   INPUTS: none
#   OUTPUTS: none
#   RETURN VALUE: none
#   SIDE EFFECTS: Change the 5th bit of CR4

enableExPaging:
#stack setup
pushl	%ebp
movl 	%esp, %ebp
#load cr4 and enable 5th bit to one to enable paging 
movl	%cr4, %eax
#0x10 has 1 in 5th bit
orl	$0x10,%eax
movl	%eax,%cr4
#stack teardown
leave
ret

