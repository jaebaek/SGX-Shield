.text
.global __clone
.type   __clone,@function
__clone:
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
	mov %rcx,(%rsi)
	syscall
	test %eax,%eax
	jnz 1f
    jmp BB0		# instrumented
BB0:		# instrumented
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
	ret
