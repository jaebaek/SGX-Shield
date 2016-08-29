/* a very simple test program */

#include <stdio.h>
#include <stdlib.h>
#include <enclave.h>
#include <time.h>

double TicksToFracSecs(unsigned long tickamount)
{
    return((double)tickamount/(double)CLOCKS_PER_SEC);
}

void enclave_main()
{
    unsigned long startticks = (unsigned long)clock();
    char* buf = (char*)malloc(0x8000);
    if (buf) {
        sprintf(buf, "The number is %d, the string is %s, another number is 0x%llx\n\0",
                137, "\"A long long long string\"\0", 0x123456789101112LLU);
        puts(buf);

        sprintf(buf, "delay = %f", TicksToFracSecs((unsigned long)clock() - startticks));
        puts(buf);
        free(buf);
    } else {
        puts("buf is NULL");
    }
    enclave_exit();
}
