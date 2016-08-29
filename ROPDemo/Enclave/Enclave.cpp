/*
 * Copyright (C) 2011-2016 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

/*
 * do_eexit:
 *   Invokes EEXIT (ENCLU) leaf instruction directly
 */
void do_eexit()
{
    unsigned long ptr = 0; /* pointer to the target function */
    unsigned long rbx = 0;
    ocall_get_ptr(&ptr);
    printf("ptr = %lx (in enclave)\n", ptr);
    __asm__ __volatile__(
            "mov %1, %%rbx\n"
            "mov %%rbx, %0\n"
            :"=m" (rbx):"m" (ptr));
    printf("rbx = %lx (in enclave)\n", rbx);
    __asm__ __volatile__(
            "mov %0, %%rbx\n"
            "xor %%rax, %%rax\n"
            "mov $0x4, %%eax\n"
            ".byte 0x0f, 0x01, 0xd7\n"
            ::"m" (ptr));
    printf("rbx = %lx (in enclave)\n", rbx);
}

#include <string.h>
/*
 * do_eexit_by_rop:
 *   Invokes EEXIT (ENCLU) leaf instruction using ROP
 */
extern "C" void gadget1();
extern "C" void gadget2();
extern "C" void gadget3();
unsigned long secret = 0x123456789;
void do_eexit_by_rop()
{
    unsigned long payload[7];
    ocall_build_payload(payload, 5 * sizeof(unsigned long));
    memcpy((void *)((unsigned long)&payload + 9 * sizeof(unsigned long)),
            payload, sizeof(unsigned long) * 5);
    __asm__ __volatile__(
            "mov %0, %%rbx\n"
            ::"r" (&secret));
}

void enclave_main()
{
    do_eexit_by_rop();
}
