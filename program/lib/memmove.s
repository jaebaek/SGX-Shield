# void *memmove(void *dest, const void *src, size_t n);

.global memmove
.type memmove,@function
.p2align 5
memmove:
	mov %rdi,%rax
	sub %rsi,%rax
	cmp %rdx,%rax
	jae memcpy

    # dep
    xor %rax, %rax
    test %rdx, %rdx
    je 1f
    jmp memmove.3

.p2align 5
memmove.3:
    lea -1(%rdi, %rdx, 1), %r13
    sub %r15, %r13
    mov %r13d, %r13d
	mov (%r15, %r13, 1), %rax
    jmp memmove.0

.p2align 5
memmove.0:
    lea -1(%rsi, %rdx, 1), %r13
    sub %r15, %r13
    mov %r13d, %r13d
	mov %rax, (%r15, %r13, 1)
    jmp memmove.1

.p2align 5
memmove.1:
    # dep

	mov %rdx,%rcx
	lea -1(%rdi,%rdx),%rdi
	lea -1(%rsi,%rdx),%rsi
	std
	rep movsb
	cld
	lea 1(%rdi),%rax
1:  ret
