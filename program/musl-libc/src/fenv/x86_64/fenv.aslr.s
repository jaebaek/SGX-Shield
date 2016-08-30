.global feclearexcept
.type feclearexcept,@function
feclearexcept:
		# maintain exceptions in the sse mxcsr, clear x87 exceptions
	mov %edi,%ecx
	and $0x3f,%ecx
	fnstsw %ax
	test %eax,%ecx
	jz 1f
    jmp BB0		# instrumented
BB0:		# instrumented
	fnclex
    jmp 1f		# instrumented
1:
	stmxcsr -8(%rsp)
	and $0x3f,%eax
	or %eax,-8(%rsp)
	test %ecx,-8(%rsp)
	jz 1f
    jmp BB1		# instrumented
BB1:		# instrumented
	not %ecx
	and %ecx,-8(%rsp)
	ldmxcsr -8(%rsp)
    jmp 1f		# instrumented
1:
	xor %eax,%eax
	ret
.global feraiseexcept
.type feraiseexcept,@function
feraiseexcept:
	and $0x3f,%edi
	stmxcsr -8(%rsp)
	or %edi,-8(%rsp)
	ldmxcsr -8(%rsp)
	xor %eax,%eax
	ret
.global __fesetround
.type __fesetround,@function
__fesetround:
	push %rax
	xor %eax,%eax
	mov %edi,%ecx
	fnstcw (%rsp)
	andb $0xf3,1(%rsp)
	or %ch,1(%rsp)
	fldcw (%rsp)
	stmxcsr (%rsp)
	shl $3,%ch
	andb $0x9f,1(%rsp)
	or %ch,1(%rsp)
	ldmxcsr (%rsp)
	pop %rcx
	ret
.global fegetround
.type fegetround,@function
fegetround:
	push %rax
	stmxcsr (%rsp)
	pop %rax
	shr $3,%eax
	and $0xc00,%eax
	ret
.global fegetenv
.type fegetenv,@function
fegetenv:
	xor %eax,%eax
	fnstenv (%rdi)
	stmxcsr 28(%rdi)
	ret
.global fesetenv
.type fesetenv,@function
fesetenv:
	xor %eax,%eax
	inc %rdi
	jz 1f
    jmp BB2		# instrumented
BB2:		# instrumented
	fldenv -1(%rdi)
	ldmxcsr 27(%rdi)
	ret
1:
	push %rax
	push %rax
	pushq $0xffff
	pushq $0x37f
	fldenv (%rsp)
	pushq $0x1f80
	ldmxcsr (%rsp)
	add $40,%rsp
	ret
.global fetestexcept
.type fetestexcept,@function
fetestexcept:
	and $0x3f,%edi
	push %rax
	stmxcsr (%rsp)
	pop %rsi
	fnstsw %ax
	or %esi,%eax
	and %edi,%eax
	ret
