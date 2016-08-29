# dep.bdr, ocall.bdr should be special symbol
.data
.global dep.bdr
.global ocall.bdr
dep.bdr:
    .zero 1
ocall.bdr:
    .zero 1
.global __enclave_exit
.type __enclave_exit, @function
.global __stack_backup
.type __stack_backup, @function
.global __rbp_backup
.type __rbp_backup, @function
__enclave_exit:
    .quad 0
__stack_backup:
    .quad 0
__rbp_backup:
    .quad 0

.text
.global _start
.type _start, @function
.p2align 5
_start:
    pop    %r13
    movabs $__enclave_exit, %r14
    mov    %r13, (%r14)
    movabs $__stack_backup, %r14
    mov    %rsp, (%r14)
    movabs $__rbp_backup, %r14
    mov    %rbp, (%r14)

    movabs $_stack, %rsp
    add    $0x400000, %rsp
    movabs $dep.bdr, %r15
    movabs $ocall.bdr, %r14

    push   %rax
    jmp enclave_main
