#include <stdio.h>
#include <stdlib.h>

unsigned long* objs;
unsigned long* objs_end;

void sort(unsigned int left, unsigned int right)
{
    unsigned int i, j;
    unsigned long pivot, tmp;
    if ((int)left >= (int)right) return; // potentially under-flow!
    pivot = objs[left];
    i = left;
    for (j = left + 1;j <= right; ++j) {
        if (objs[j] < pivot) {
            ++i;
            tmp = objs[j]; objs[j] = objs[i]; objs[i] = tmp;
            tmp = objs_end[j]; objs_end[j] = objs_end[i]; objs_end[i] = tmp;
        }
    }
    tmp = objs[left]; objs[left] = objs[i]; objs[i] = tmp;
    tmp = objs_end[left]; objs_end[left] = objs_end[i]; objs_end[i] = tmp;
    sort(left, i-1);
    sort(i+1, right);
}

int check_overlap()
{
    unsigned int i;
    for (i = 1;i < 0x1000;++i) {
        if (objs_end[i-1] > objs[i])
            return 1;
    }
    return 0;
}

char __tmp[64] = {0};
#define dlog(...) sprintf(__tmp, __VA_ARGS__); puts(__tmp); memset(__tmp, 0, 64)

#include <enclave.h>
void enclave_main()
{
    unsigned int i;
    objs = (unsigned long*)malloc(0x1000 * sizeof(unsigned long));
    objs_end = (unsigned long*)malloc(0x1000 * sizeof(unsigned long));
    for (i = 0;i < 0x1000; ++i) {
        size_t size = i % 5 + 5;
        objs[i] = (unsigned long)malloc(size);
        objs_end[i] = objs[i]+size;
    }

    sort(0, 0x1000-1);
    if (check_overlap())
        puts("overlap exist");
    else
        puts("no overlap");

    for (i = 0;i < 0x1000; ++i)
        free((void*)objs[i]);
    enclave_exit();
}
