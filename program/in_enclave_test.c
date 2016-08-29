#include "benchmark/bm_malloc_simple.c"

/* If nothing is executed */
//__asm__ __volatile__ (
//        ".text\n"
//        ".global enclave_main\n"
//        ".type enclave_main, @function\n"
//        "enclave_main:\n"
//        "push   %rbp\n"
//        "mov    %rsp, %rbp\n"
//        "movabs $__rbp_backup, %r14\n"
//        "mov    (%r14), %rbp\n"
//        "movabs $__stack_backup, %r14\n"
//        "mov    (%r14), %rsp\n"
//        "movabs $__enclave_exit, %r14\n"
//        "jmp    *(%r14)"
//        );
