.bundle_align_mode 6
.global _longjmp
.global longjmp
.type _longjmp,@function
.type longjmp,@function
_longjmp:
longjmp:
    .p2align    6, 0x90
	mov %rsi,%rax           /* val will be longjmp return */
	test %rax,%rax
	jnz 1f
    jmp BB0		# instrumented
BB0:		# instrumented
    .p2align    6, 0x90
	inc %rax                /* if val==0, val=1 per longjmp semantics */
    jmp 1f		# instrumented
1:
    .p2align    6, 0x90
	mov (%rdi),%rbx         /* rdi is the jmp_buf, restore regs from it */
	mov 8(%rdi),%rbp
	mov 16(%rdi),%r12
	mov 24(%rdi),%r13
	mov 32(%rdi),%r14
	mov 40(%rdi),%r15
	mov 48(%rdi),%rdx       /* this ends up being the stack pointer */
	mov %rdx,%rsp
	mov 56(%rdi),%rdx       /* this is the instruction pointer */
    .bundle_lock
    pushfq
    pushq   %rax
    movq    $sfi.boundary, %rax
    cmpq    %rax, %rdx
    jl      sfi.fault
    andb    $-64, %dl
    popq    %rax
    popfq 
	jmp *%rdx               /* goto saved address without altering rsp */
    .bundle_unlock
