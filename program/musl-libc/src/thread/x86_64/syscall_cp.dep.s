.bundle_align_mode 6
.text
.global __syscall_cp_asm
.type   __syscall_cp_asm,@function
__syscall_cp_asm:
.global __cp_begin
__cp_begin:
    .p2align    6, 0x90
    subq $0x100, %rsp
	mov (%rdi),%eax
	test %eax,%eax
	jnz __cancel
    jmp BB0		# instrumented
BB0:		# instrumented
    .p2align    6, 0x90
	mov %rdi,%r11
	mov %rsi,%rax
	mov %rdx,%rdi
	mov %rcx,%rsi
	mov %r8,%rdx
	mov %r9,%r10
	mov 8(%rsp),%r8
	mov 16(%rsp),%r9
    .bundle_lock
    pushfq
    pushq   %rax  
    pushq   %rdx  
    leaq    8(%rsp), %rax
    movq    $dep.boundary, %rdx
    cmpq    %rdx, %rax
    jl      dep.fault
    popq    %rdx  
    popq    %rax  
    popfq 
	mov %r11,8(%rsp)
    .bundle_unlock
	syscall
    jmp __cp_end		# instrumented
.global __cp_end
__cp_end:
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
