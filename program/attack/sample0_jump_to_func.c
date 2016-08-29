/* a very simple test program */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <enclave.h>

void print_attacked(void)
{
    puts("RET_TO_FUNC success");
}

void ret_to_func()
{
    unsigned long input;
    read(0, (void *)&input, sizeof(unsigned long));
    ((unsigned long *)&input)[2] = input;
}

void enclave_main()
{
    push_gadget((unsigned long)print_attacked);
    ret_to_func();
    enclave_exit();
}
