.section .text
.global attacked
.type attacked, @function
attacked:
    mov $stack_for_attack, %rsp
    add $0x40000, %rsp
    call print_attack_msg
