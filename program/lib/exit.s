.text
.global exit
.type exit, @function
.p2align 5
exit:
    movabs $__rbp_backup, %r14
    mov    (%r14), %rbp
    movabs $__stack_backup, %r14
    mov    (%r14), %rsp
    movabs $__enclave_exit, %r14
    pop    %rax
    jmp    *(%r14)
