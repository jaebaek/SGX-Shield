/* a very simple test program */

#include <stdio.h>
#include <stdlib.h>

#include <enclave.h>
void enclave_main()
{
    /* the printf internal uses complicated ABIs */
    printf("The number is %d, the string is %s, another number is 0x%llx\n",
            137, "\"A long long long string\"", 0x123456789101112LLU);
    fflush( stdout );
    enclave_exit();
}
