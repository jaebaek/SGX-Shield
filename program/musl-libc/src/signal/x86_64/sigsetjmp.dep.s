.bundle_align_mode 6
.global sigsetjmp
.type sigsetjmp,@function
sigsetjmp:
    .p2align    6, 0x90
	andl %esi,%esi
    .bundle_lock
    pushfq
    pushq   %rax
    pushq   %rdx
    leaq    64(%rdi), %rax
    movq    $dep.boundary, %rdx
    cmpq    %rdx, %rax
    jl  dep.fault
    popq    %rdx
    popq    %rax
    popfq
	movq %rsi,64(%rdi)
    .bundle_unlock
	jz 1f
    jmp BB0		# instrumented
BB0:		# instrumented
    .p2align    6, 0x90
	pushq %rdi
	leaq 72(%rdi),%rdx
	xorl %esi,%esi
	movl $2,%edi
	call sigprocmask
    .p2align    6, 0x90
	popq %rdi
    jmp 1f		# instrumented
1:
    .p2align    6, 0x90
	jmp setjmp
