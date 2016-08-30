.section .text
.global attacked
attacked:
    mov $stack_for_attack, %rsp
    add $0x1000, %rsp
    call print_attack
