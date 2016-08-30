.global sigsetjmp
.type sigsetjmp,@function
sigsetjmp:
	andl %esi,%esi
	movq %rsi,64(%rdi)
	jz 1f
    jmp BB0		# instrumented
BB0:		# instrumented
	pushq %rdi
	leaq 72(%rdi),%rdx
	xorl %esi,%esi
	movl $2,%edi
	call sigprocmask
	popq %rdi
    jmp 1f		# instrumented
1:
	jmp setjmp
