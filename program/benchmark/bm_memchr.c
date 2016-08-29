/* a very simple test program */

#include <stdio.h>
#include <string.h>

char *str = "This is a long string";

#include <enclave.h>
void enclave_main()
{
    char *p = memchr(str, 'l', strlen(str));
    puts(p);
    enclave_exit();
}
