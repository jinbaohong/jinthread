.global tswitch, running, scheduler
tswitch:
SAVE:
	pushal # push all general regs
	pushfl # push flag
	# Now, esp is point to eflag.
	movl running, %ebx
	# 0(%ebx) == running_proc.next
	# 4(%ebx) == running_proc.ksp
	movl %esp, 4(%ebx) # integers in GCC are 4 bytes
FIND:
	call scheduler
RESUME:
	movl running, %ebx
	movl 4(%ebx), %esp # Every user thread has its own esp
	popfl # pop and push just follow the esp, it's not necessary for esp to belong to process.
	popal
	ret # return address is on stack?