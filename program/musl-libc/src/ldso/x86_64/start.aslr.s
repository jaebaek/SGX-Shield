.text
.global _start
_start:
	mov (%rsp),%rdi
	lea 8(%rsp),%rsi
	call __dynlink
	pop %rdi
    jmp 1f		# instrumented
1:
	dec %edi
	pop %rsi
	cmp $-1,%rsi
	jz 1b
    jmp BB0		# instrumented
BB0:		# instrumented
	inc %edi
	push %rsi
	push %rdi
	xor %edx,%edx
	jmp *%rax
