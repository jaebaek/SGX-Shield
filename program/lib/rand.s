# rand_internal should be special symbol
.text
.bundle_align_mode 5
.global srand
.type srand, @function
.p2align 5
srand:
    ret

.global rand
.type rand, @function
.p2align 5
rand:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock

    call rand_internal
    jmp rand.0

    .p2align 5
rand.0:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock
    jmp rand.1

    .p2align 5
rand.1:
    movabs $ocall.bdr, %r14
    movabs $dep.bdr, %r15
    jmp rand.2

    .p2align 5
rand.2:
    ret

// rand_internal should be changed to ocall_rand in windows
.global rand_internal
.type rand_internal, @function
.p2align 5
rand_internal:
    ret
