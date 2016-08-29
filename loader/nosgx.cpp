#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <stdlib.h>

#include "elf.h"        /* ELF */

void ocall_print_string(const char* str)
{
    puts(str);
}

void sgx_read_rand(unsigned char *buf, size_t size)
{
    for (int i = 0; i < size; ++i) {
        buf[i] = (unsigned char)rand();
    }
}

#ifdef LD_DEBUG
#define dlog(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#else
#define dlog(...)
#endif

extern char __elf_end;          /* defined in the linker script */
unsigned char heap_space[0x10000];
#define _HEAP_BASE (((addr_t)heap_space + 0xfff) & ~0xfff)

static unsigned long __sgx_data_ofs = 0x2027000;
void sgx_push_gadget(unsigned long a) {}

#define PTRACE 1

#if PTRACE
#include <sys/ptrace.h>     /* ptrace */
#include <sys/user.h>       /* user_regs_struct */
#include <sys/types.h>      /* wait */
#include <sys/wait.h>
#include <limits.h>
#include <string.h>

#include <string>
using namespace std;

const char *ptrace_targets[] = {
    "DoEmFloatIteration",
    "DoNumSortIteration",
    "DoStringSortIteration",
    "DoBitfieldIteration",
    "DoFPUTransIteration",
    "DoAssignIteration",
    "DoIDEAIteration",
    "DoHuffIteration",
    "DoNNetIteration",
    "DoLUIteration",
};
const size_t ptrace_targets_size = sizeof(ptrace_targets) / sizeof(char *);
unsigned ptrace_sym[ptrace_targets_size] = {0};
unsigned long ptrace_bt[ptrace_targets_size] = {0};
long ptrace_bt_word[ptrace_targets_size] = {0};

void find_ptrace_target(const char *name, unsigned sym)
{
    for (size_t i = 0; i < ptrace_targets_size; ++i) {
        if (!strcmp(name, ptrace_targets[i])) {
            ptrace_sym[i] = sym;
            break;
        }
    }
}

void check_breakpoint(unsigned sym, unsigned long addr)
{
    for (size_t i = 0; i < ptrace_targets_size; ++i) {
        if (ptrace_sym[i] == sym) {
            ptrace_bt[i] = addr - 1;
            printf("%lx -- %s\n", ptrace_bt[i], ptrace_targets[i]);
            break;
        }
    }
}

int delete_breakpoint(pid_t pid, unsigned long rip)
{
    for (size_t i = 0; i < ptrace_targets_size; ++i) {
        if (ptrace_bt[i] == rip) {
            printf("Hit ptrace_bt[%lu] = %lx (%s)\n", i, ptrace_bt[i], ptrace_targets[i]);
            ptrace(PTRACE_POKEDATA, pid, (void *)ptrace_bt[i], ptrace_bt_word[i]);
            return (int)i;
            break;
        }
    }
    return -1;
}

long set_break(pid_t pid, unsigned long addr)
{
    long word = ptrace(PTRACE_PEEKDATA, pid, (void *)addr, 0);
    ptrace(PTRACE_POKEDATA, pid, (void *)addr, 0x27);
    long tmp = ptrace(PTRACE_PEEKDATA, pid, (void *)addr, 0);
    return word;
}

#define GET_OPCODE(i) (((unsigned char *)rip)[i])
extern char __sgx_code;         /* defined in the linker script */
unsigned sym_count[0x100000] = {0};
unsigned long instr_count = 0;
inline void do_trace(unsigned long rip)
{
    ++instr_count;
    if (!(rip & 31)) {
        rip = (rip - (unsigned long)&__sgx_code) / 32;
        if (rip < 0x100000) {
            if (sym_count[rip] == UINT_MAX)
                printf("sym_count[%lu] overflowed!!\n", rip);
            ++sym_count[rip];
        }
    }
}
unsigned long last_rip = 0;
unsigned long jmp_count = 0;
inline void do_trace2(unsigned long rip)
{
    ++instr_count;
    if (rip + 32 <= last_rip || last_rip + 32 <= rip)
        ++jmp_count;
    last_rip = rip;
}
void do_print_trace(FILE *fp);
void do_print_trace2(FILE *fp);

void run_with_ptrace(void (*f)())
{
    pid_t pid = fork();
    if (pid == -1) {            /* fail */
        printf("PTRACE fail at fork()\n");
        exit(1);
    } else if (pid == 0) {      /* child */
        sleep(1);
        __asm__ __volatile__( "push %%r13\n" "push %%r14\n" "push %%r15\n" ::);
        f();
        __asm__ __volatile__( "pop %%r15\n" "pop %%r14\n" "pop %%r13\n" ::);
    } else {                    /* parent */
        long ret = ptrace(PTRACE_ATTACH, pid, 0, 0);
        if (ret) {
            printf("ptrace fails: %ld\n", ret);
            exit(1);
        }
        wait(NULL);

        for (size_t i = 0; i < 10; ++i) {
            printf("ptrace_bt[%lu] = %lx (%s)\n",
                    i, ptrace_bt[i], ptrace_targets[i]);
            ptrace_bt_word[i] = set_break(pid, ptrace_bt[i]);
        }

        for (size_t i = 0; i < 10; ++i) {
            /* continue */
            ptrace(PTRACE_CONT, pid, 0, 0);
            wait(NULL);

            /* identify which breakpoint hits and delete it */
            struct user_regs_struct regs = {0};
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            printf("fault at %llx\n", regs.rip);

            int bt = delete_breakpoint(pid, regs.rip);
            if (bt == -1) {
                fprintf(stderr, "The is no break point hit\n");
                exit(1);
            }

            /* open the ptrace log file */
            string ptrace_file("tmp/");
            FILE *fp = fopen((ptrace_file+ptrace_targets[bt]+".ptrace").c_str(), "w+");

            /* finish */
            unsigned long until = regs.rip + 5;
            while (regs.rip != until) {
                ptrace(PTRACE_GETREGS, pid, 0, &regs);
                do_trace2(regs.rip);
                ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
                wait(NULL);
            }
            do_print_trace2(fp);

            /* flush and close the file */
            int flushed = fflush(fp);
            if (flushed)
                fprintf(stderr, "fflush fails\n");
            fclose(fp);
        }

        ptrace(PTRACE_CONT, pid, 0, 0);
        ptrace(PTRACE_DETACH, pid, 0, 0);
        wait(NULL);
    }
}
#endif /* PTRACE */

#define NOSGX 1
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
typedef void (*sighandler_t)(int);
#include "Enclave/loader.cpp"

#if PTRACE
unsigned v_to_sym[0x100000];
void do_print_trace(FILE *fp)
{
    static bool init = false;
    printf("do_print_trace: %lx\n", instr_count);
    fprintf(fp, "do_print_trace: %lx\n", instr_count);
    instr_count = 0;
    if (!init) {
        for (unsigned i = 1; i < n_symtab; ++i) {
            if (symtab[i].st_shndx < pehdr->e_shnum
                    && (pshdr[symtab[i].st_shndx].sh_flags & SHF_EXECINSTR)
                    && (symtab[i].st_value - (unsigned long)&__sgx_code) < 0x2000000)
                v_to_sym[(symtab[i].st_value - (unsigned long)&__sgx_code) / 32] = i;
        }
        init = true;
    }
    for (unsigned i = 0; i < 0x100000; ++i) {
        if (sym_count[i])
            fprintf(fp, "%04u (%s): %u\n", v_to_sym[i],
                    &strtab[symtab[v_to_sym[i]].st_name], sym_count[i]);
        sym_count[i] = 0;
    }
}
void do_print_trace2(FILE *fp)
{
    static bool init = false;
    printf("do_print_trace: %lx, %lx\n", instr_count, jmp_count);
    fprintf(fp, "do_print_trace: %lx, %lx\n", instr_count, jmp_count);
    instr_count = 0;
    last_rip = 0;
    jmp_count = 0;
}
#endif /* PTRACE */

#include <sys/mman.h>
int main(int argc, const char *argv[])
{
    void *sgx_data = mmap(_SGXDATA_BASE, _SGX_SIZE, PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (sgx_data == MAP_FAILED) {
        fprintf(stderr, "mmap fails\n");
        return 1;
    }
    __sgx_data_ofs = (unsigned long)sgx_data - (unsigned long)_SGXCODE_BASE;
    enclave_main();
    return 0;
}
