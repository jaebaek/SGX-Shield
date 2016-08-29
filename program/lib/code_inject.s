.section .text
.global memcpy_nodep
.type memcpy_nodep, @function
.p2align 5
memcpy_nodep:
    push    %rbp
    jmp     memcpy_nodep_loop

.p2align 5
memcpy_nodep_loop:
    cmp     $0x0, %rdx
    je      memcpy_nodep_done
    dec     %rdx
    mov     (%rsi, %rdx, 1), %r13
    mov     %r13, (%rdi, %rdx, 1)
    jmp     memcpy_nodep_loop

.p2align 5
memcpy_nodep_done:
    pop     %rbp
    ret
