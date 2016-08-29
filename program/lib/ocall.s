.text
.bundle_align_mode 5
.global sgx_ocall
.type sgx_ocall, @function
.p2align 5
sgx_ocall:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock

    call sgx_ocall.loader
    jmp sgx_ocall.0

    .p2align 5
sgx_ocall.0:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock
    jmp sgx_ocall.1

    .p2align 5
sgx_ocall.1:
    movabs $ocall.bdr, %r14
    movabs $dep.bdr, %r15
    ret

# this must be replaced by do_ocall() in loader
.global sgx_ocall.loader
.type sgx_ocall.loader, @function
.p2align 5
sgx_ocall.loader:
    ret
