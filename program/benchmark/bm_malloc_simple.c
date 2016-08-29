/* a very simple test program */

#include <stdio.h>
#include <stdlib.h>
#include <enclave.h>

void enclave_main()
{
    char* buf = (char*)malloc(0x8000);
    if (buf) {
        sprintf(buf, "The number is %d, the string is %s, another number is 0x%llx\n\0",
                137, "\"A long long long string\"\0", 0x123456789101112LLU);
        puts(buf);
        free(buf);
    } else {
        puts("buf is NULL");
    }
    enclave_exit();
}
