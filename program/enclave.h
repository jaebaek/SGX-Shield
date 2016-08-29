#ifndef __ENCLAVE_H
#define __ENCLAVE_H

#include <stdlib.h>

#define MAX_OCALL_GET_LINE 64

#define enclave_exit() exit(0)

void enclave_main();
void *memalign(size_t alignment, size_t size);
void ocall_open(char *name, int *fd);
size_t get_line(int fd, char *buf);
void push_gadget(unsigned long gadget);

#endif
