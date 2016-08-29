/*
 * Jaebaek: This is a wrapper file to provide Enclave_u.c
 *          with C header files.
 *          Do not insert this to Enclave_u.c!
 *          Enclave_u.c can be re-generated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "ocall_type.h"

typedef void (*sighandler_t)(int);

#include "Enclave_t.c"
