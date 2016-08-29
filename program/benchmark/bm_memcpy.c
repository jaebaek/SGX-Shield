/* a very simple test program */

#include <stdio.h>
#include <stdlib.h>
#include <enclave.h>

void enclave_main()
{
    char buf[32] = {0};
    memcpy(buf, "This is a long string to check!\0", 32);
    puts(buf);
    enclave_exit();
}
