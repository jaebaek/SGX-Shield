// get_line (int fd, char *buf)
.text
.bundle_align_mode 5
.global get_line
.type get_line, @function
.p2align 5
get_line:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock

    call get_line_internal
    jmp get_line.0

    .p2align 5
get_line.0:
    .bundle_lock
    mov %rsp, %r14
    movabs $__stack_backup, %r13
    mov (%r13), %rsp
    mov %r14, (%r13)
    .bundle_unlock
    jmp get_line.1

    .p2align 5
get_line.1:
    movabs $ocall.bdr, %r14
    movabs $dep.bdr, %r15
    ret

.p2align 5
.global get_line_internal
.type get_line_internal, @function
get_line_internal:
    ret
