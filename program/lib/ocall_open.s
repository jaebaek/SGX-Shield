.text
.bundle_align_mode 5
.global ocall_open
.type ocall_open, @function
.p2align 5
ocall_open:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock

    call ocall_open_internal
    jmp ocall_open.0

    .p2align 5
ocall_open.0:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock
    jmp ocall_open.1

    .p2align 5
ocall_open.1:
    movabs $ocall.bdr, %r14
    movabs $dep.bdr, %r15
    ret

.p2align 5
.global ocall_open_internal
.type ocall_open_internal, @function
ocall_open_internal:
    ret
