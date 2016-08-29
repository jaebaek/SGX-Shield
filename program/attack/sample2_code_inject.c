/* a very simple test program */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <enclave.h>

char shellcode[] =
"\x48\xc7\xc0\x04\x00\x00\x00"    // mov    $0x4,%rax
"\x48\xc7\xc7\x88\x42\x60\x00"    // mov    $0x604288,%rdi -- attack_msg
"\x48\xc7\xc3\xbc\x0f\x40\x00"    // mov    $0x400fbc,%rbx -- attacked
"\x0f\x01\xd7";                   // ENCLU

const char *msg = "This is a test!";

void code_injection_with_dep()
{
    void (*code)() = (void (*)())((unsigned long)enclave_main + 0x100000);
    memcpy(code, shellcode, sizeof(shellcode));
    code();
}

extern void memcpy_nodep(void *dst, void *src, size_t n);

void code_injection()
{
    void (*code)() = (void (*)())((unsigned long)enclave_main + 0x100000);
    memcpy_nodep(code, shellcode, sizeof(shellcode));
    code();
}

void enclave_main()
{
    code_injection();
    puts("not reached");
    enclave_exit();
}
