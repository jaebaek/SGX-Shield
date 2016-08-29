# void *memset(void *rdi (*s), int esi (c), size_t rdx (n));

.global memset
.type memset,@function
.p2align 5
memset:
    # dep
    mov %rdx, %rax
    test %rdx, %rdx
    jz 2f
    lea -1(%rdi, %rdx, 1), %r13
    sub %r15, %r13
    mov %r13d, %r13d
	mov %esi, (%r15, %r13, 1)
    # dep
    jmp memset.0

.p2align 5
memset.0:
	and $0xff,%esi
	mov $0x101010101010101,%rax
	mov %rdx,%rcx
	mov %rdi,%r8
	imul %rsi,%rax
    jmp memset.1

.p2align 5
memset.1:
	cmp $16,%rcx
	jb 1f

	mov %rax,-8(%rdi,%rcx)
	shr $3,%rcx
	rep
	stosq
	mov %r8,%rax
	ret

.p2align 5
1:	test %ecx,%ecx
	jz 2f

	mov %al,(%rdi)
	mov %al,-1(%rdi,%rcx)
	cmp $2,%ecx
	jbe 2f

	mov %al,1(%rdi)
    jmp memset.2

.p2align 5
memset.2:
	mov %al,-2(%rdi,%rcx)
	cmp $4,%ecx
	jbe 2f

	mov %eax,(%rdi)
	mov %eax,-4(%rdi,%rcx)
    jmp memset.3

.p2align 5
memset.3:
	cmp $8,%ecx
	jbe 2f

	mov %eax,4(%rdi)
	mov %eax,-8(%rdi,%rcx)

2:	mov %r8,%rax
	ret
