.section .text
.global gadget1
gadget1:
    pop %rax
    add $0x5, %rax
    ret

.global gadget2
gadget2:
    mov (%rbx), %rdi
    pop %rbx
    ret

.global gadget3
gadget3:
    .byte 0x0f, 0x01, 0xd7
