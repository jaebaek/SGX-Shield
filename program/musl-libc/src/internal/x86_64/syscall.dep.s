.bundle_align_mode 6
.global __syscall
.type __syscall,@function
__syscall:
    .p2align    6, 0x90
    subq $0x100, %rsp
	movq %rdi,%rax
	movq %rsi,%rdi
	movq %rdx,%rsi
	movq %rcx,%rdx
	movq %r8,%r10
	movq %r9,%r8
	movq 8(%rsp),%r9
	syscall
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
