.text
.global __unmapself
.type   __unmapself,@function
__unmapself:
    .p2align    6, 0x90
	movl $11,%eax   /* SYS_munmap */
	syscall         /* munmap(arg2,arg3) */
	xor %rdi,%rdi   /* exit() args: always return success */
	movl $60,%eax   /* SYS_exit */
	syscall         /* exit(0) */
