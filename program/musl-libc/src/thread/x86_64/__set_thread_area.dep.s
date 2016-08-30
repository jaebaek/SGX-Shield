.bundle_align_mode 6
.text
.global __set_thread_area
.type __set_thread_area,@function
__set_thread_area:
    .p2align    6, 0x90
	mov %rdi,%rsi           /* shift for syscall */
	movl $0x1002,%edi       /* SET_FS register */
	movl $158,%eax          /* set fs segment to */
	syscall                 /* arch_prctl(SET_FS, arg)*/
    .bundle_lock
    pushfw
    pushq   %rdx
    movq    $sfi.boundary, %rdx
    cmpq    %rdx, 10(%rsp)
    jl      sfi.fault
    andb    $-64, 10(%rsp)
    popq    %rdx
    popfw 
    ret
    .bundle_unlock
