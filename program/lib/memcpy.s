# void *memcpy(void *dest, const void *src, size_t n);

.global memcpy
.type memcpy,@function
.p2align 5
memcpy:
    # dep
    mov %rdi, %rax
    test %rdx, %rdx
    jz 3f
    lea -1(%rdi, %rdx, 1), %r13
    sub %r15, %r13
    mov %r13d, %r13d
	mov (%r15, %r13, 1), %rax
    jmp memcpy.5

.p2align 5
memcpy.5:
    lea -1(%rsi, %rdx, 1), %r13
    sub %r15, %r13
    mov %r13d, %r13d
	mov %rax, (%r15, %r13, 1)
    # dep
    jmp memcpy.0

.p2align 5
memcpy.0:
	mov %rdi,%rax
	cmp $8,%rdx
	jc 1f
	test $7,%edi
	jz 1f
2:	movsb
    jmp memcpy.1

.p2align 5
memcpy.1:
	dec %rdx
	test $7,%edi
	jnz 2b
1:	mov %rdx,%rcx
	shr $3,%rcx
	rep
	movsq
    jmp memcpy.2

.p2align 5
memcpy.2:
	and $7,%edx
	jz 3f
2:	movsb
	dec %edx
	jnz 2b
3:	ret
