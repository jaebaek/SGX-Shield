.bundle_align_mode 6
.text
.global __clone
.type   __clone,@function
__clone:
    .p2align    6, 0x90
    subq $0x100, %rsp
	xor %eax,%eax
	mov $56,%al
	mov %rdi,%r11
	mov %rdx,%rdi
	mov %r8,%rdx
	mov %r9,%r8
	mov 8(%rsp),%r10
	mov %r11,%r9
	and $-16,%rsi
	sub $8,%rsi
    .bundle_lock
    pushfq
    pushq   %rax  
    pushq   %rdx
    leaq    (%rsi), %rax
    movq    $dep.boundary, %rdx
    cmpq    %rdx, %rax
    jl      dep.fault
    popq    %rdx
    popq    %rax  
    popfq 
	mov %rcx,(%rsi)
    .bundle_unlock
	syscall
	test %eax,%eax
	jnz 1f
    jmp BB0		# instrumented
BB0:		# instrumented
    .p2align    6, 0x90
	xor %ebp,%ebp
	pop %rdi
	call *%r9
	mov %eax,%edi
	xor %eax,%eax
	mov $60,%al
	syscall
	hlt
    jmp 1f		# instrumented
1:
    .p2align    6, 0x90
    .bundle_lock
    pushfw
    pushq   %rdx
    movq    $sfi.boundary, %rdx
    cmpq    %rdx, 10(%rsp)
    jl      sfi.fault
    andb    $-64, 10(%rsp)
    popq    %rdx
    popfw 
    addq $0x100, %rsp
    ret
    .bundle_unlock
