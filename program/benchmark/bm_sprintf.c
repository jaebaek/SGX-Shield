/* a very simple test program */

#include <stdio.h>
#include <stdlib.h>

char buf[1024] = {0};

#include <enclave.h>
void enclave_main()
{
    /* the printf internal uses complicated ABIs */
    sprintf(buf, "The number is %d, the string is %s, another number is 0x%llx\n",
            137, "\"A long long long string\"", 0x123456789101112LLU);
    puts(buf);
    enclave_exit();
}
