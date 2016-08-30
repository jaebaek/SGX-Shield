.bundle_align_mode 6
.text
.global _start
_start:
    .p2align    6, 0x90
	mov (%rsp),%rdi
	lea 8(%rsp),%rsi
	call __dynlink
    .p2align    6, 0x90
	pop %rdi
    jmp 1f		# instrumented
1:
    .p2align    6, 0x90
	dec %edi
	pop %rsi
	cmp $-1,%rsi
	jz 1b
    jmp BB0		# instrumented
BB0:		# instrumented
    .p2align    6, 0x90
	inc %edi
	push %rsi
	push %rdi
	xor %edx,%edx
    .bundle_lock
    pushfq
    pushq   %rdx
    movq    $sfi.boundary, %rax
    cmpq    %rdx, (%rsp)
    jl      sfi.fault
    andb    $-64, %al
    popq    %rdx
    popfq 
	jmp *%rax
    .bundle_unlock
