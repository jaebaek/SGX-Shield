#include <iostream>
#include <elf.h>

#include <err.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <endian.h>
#if BYTE_ORDER == BIG_ENDIAN
# define byteorder ELFDATA2MSB
#elif BYTE_ORDER == LITTLE_ENDIAN
# define byteorder ELFDATA2LSB
#else
# error "Unknown BYTE_ORDER " BYTE_ORDER
# define byteorder ELFDATANONE
#endif

using namespace std;

void read_ehdr(int fd, Elf64_Ehdr *ehdr)
{
    static const unsigned char expected[EI_NIDENT] =
    {
        [EI_MAG0] = ELFMAG0,
        [EI_MAG1] = ELFMAG1,
        [EI_MAG2] = ELFMAG2,
        [EI_MAG3] = ELFMAG3,
        [EI_CLASS] = ELFCLASS64,
        [EI_DATA] = byteorder,
        [EI_VERSION] = EV_CURRENT,
        [EI_OSABI] = ELFOSABI_SYSV,
        [EI_ABIVERSION] = 0
    };

    if (read(fd, ehdr, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr))
        err(EXIT_FAILURE, "Ehdr size");

    if (memcmp(ehdr->e_ident, expected, EI_ABIVERSION) != 0
            || ehdr->e_ident[EI_ABIVERSION] != 0
            || memcmp (&ehdr->e_ident[EI_PAD],
                &expected[EI_PAD],
                EI_NIDENT - EI_PAD) != 0)
        err(EXIT_FAILURE, "Ehdr ident");

    if (ehdr->e_version != EV_CURRENT)
        err(EXIT_FAILURE, "Ehdr version");

    /* ELF format check - relocatable */
    if (ehdr->e_type != ET_REL)
        err(EXIT_FAILURE, "Ehdr not relocatable");

    /* check the architecture - currently only support x86_64 */
    if (ehdr->e_machine != EM_X86_64)
        err(EXIT_FAILURE, "Ehdr not x86_64");
}

Elf64_Shdr *shdr_array = NULL;
char *strtab;
Elf64_Sym *sym;
Elf64_Rela *reltab = NULL;
size_t reltab_size = 0;

void read_symtab(int fd, Elf64_Ehdr *ehdr, const string& file)
{
    if (ehdr->e_shentsize != sizeof (Elf64_Shdr))
        err(EXIT_FAILURE, "Shdr entry size");

    shdr_array = new Elf64_Shdr[ehdr->e_shnum];
    if (!shdr_array)
        err(EXIT_FAILURE, "Shdr new");
    if (pread(fd, shdr_array, ehdr->e_shnum * sizeof(Elf64_Ehdr), ehdr->e_shoff)
            != ehdr->e_shnum * sizeof(Elf64_Ehdr))
        err(EXIT_FAILURE, "Shdr pread");

    for (int i = 0; i < ehdr->e_shnum; ++i) {
        if (shdr_array[i].sh_type == SHT_SYMTAB) {
            if (!shdr_array[i].sh_size) continue;

            /* read the linked string table */
            size_t size = shdr_array[shdr_array[i].sh_link].sh_size;
            strtab = new char[size];
            if (!strtab)
                err(EXIT_FAILURE, "Shdr symtab strtab new");
            if (pread(fd, strtab, size, shdr_array[shdr_array[i].sh_link].sh_offset) != size)
                err(EXIT_FAILURE, "Shdr symtab strtab");

            /* read the symbol table */
            size = shdr_array[i].sh_size;
            sym = (Elf64_Sym *)malloc(size);
            if (!sym)
                err(EXIT_FAILURE, "Shdr symtab malloc");
            if (pread(fd, sym, size, shdr_array[i].sh_offset) != size)
                err(EXIT_FAILURE, "Shdr symtab");
        }

        if (shdr_array[i].sh_type == SHT_RELA) {
            if (!shdr_array[i].sh_size) continue;

            /* read the relocation table */
            const size_t size = shdr_array[i].sh_size;
            void *buf = NULL;

            if (reltab_size) {
                buf = malloc(reltab_size + size);
                memcpy(buf, reltab, reltab_size);
                free(reltab);
                reltab = (Elf64_Rela *)buf;
            } else {
                reltab = (Elf64_Rela *)malloc(size);
            }

            buf = (void *)((unsigned long)reltab + reltab_size);
            reltab_size += size;

            if (!buf)
                err(EXIT_FAILURE, "Shdr reltab malloc");
            if (pread(fd, buf, size, shdr_array[i].sh_offset) != size)
                err(EXIT_FAILURE, "Shdr reltab");
        }
    }
}

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <map>

/* effective instructions */
std::map<void*, Elf64_Sym*> eff_inst; /* addr to sym */
std::vector<void*> sym_to_addr;

void handle_sym(const string& log)
{
    stringstream ss(log);
    string sig;
    ss >> sig;
    cout << sig << "[";

    int i;
    ss >> i;
    cout << i << "] (" << &strtab[sym[i].st_name] << ") ";

    void* addr;
    ss >> addr;
    printf("%p\n", addr);
    eff_inst[addr] = &sym[i];
    sym_to_addr.push_back(addr);
}

bool is_hex(const string& log)
{
    for (int i = 0; i < log.size(); ++i) {
        if (!('0' <= log[i] && log[i] <= '9')
            && !('a' <= log[i] && log[i] <= 'f'))
            return false;
    }
    return true;
}

#include <cstdlib>
void read_dlog(const string& dlog)
{
    ifstream fin(dlog.c_str());
    string log;
    bool shown = false;
    while (getline(fin, log)) {
        if (!strncmp(log.c_str(), "sym ", 4)) {
            shown = true;
            break;
        }
    }

    if (shown)
        handle_sym(log);

    while (getline(fin, log)) {
        if (!strncmp(log.c_str(), "sym ", 4)) {
            handle_sym(log);
        } else if (log.size() == 8 && is_hex(log)) {
            void* addr = (void*)strtol(log.c_str(),0,16);
            std::map<void*, Elf64_Sym*>::iterator it = eff_inst.upper_bound(addr);
            --it;
            if (it != eff_inst.end()
                    && (unsigned long)it->first <= (unsigned long)addr
                    && (unsigned long)addr
                    < (unsigned long)it->first+(unsigned long)it->second->st_size) {
                Elf64_Sym *tmp = it->second;
                printf("%p (%s) %lx\n", addr, &strtab[tmp->st_name],
                        (unsigned long)addr - (unsigned long)it->first + tmp->st_value);
            } else {
                printf("%p\n", addr);
            }
        }
    }
    fin.close();
}

#include <cassert>

void handle_rel(const string& log, int& cnt)
{
    cout << log << endl;

    stringstream ss(log);
    string sig;
    ss >> sig;
    ss >> sig; /* "dst_sym:" */

    int i = 0;
    unsigned long value[8];
    while(!ss.eof()) {
        ss >> value[i++];
        if (i >= 8) break;
    }

    unsigned long src = 0;
    for (int j = i-1; j >= 0; --j) {
        src = (src << sizeof(unsigned char)) + value[j];
    }

    Elf64_Word src_sym = ELF64_R_SYM(reltab[cnt++].r_info);
    printf("%p, %lx\n", sym_to_addr[src_sym], src);
    assert(sym_to_addr[src_sym] == (void*)src);
}

int relstrsz = 0;

void verify_reloc(const string& dlog)
{
    ifstream fin(dlog.c_str());
    string log;
    bool shown = false;
    while (getline(fin, log)) {
        if (log.size() == relstrsz && !strncmp(log.c_str(), "rel[", 4)) {
            shown = true;
            break;
        }
    }

    int cnt = 1;
    if (shown) {
        handle_rel(log, cnt);
    }
    while (getline(fin, log)) {
        if (log.size() == relstrsz && !strncmp(log.c_str(), "rel[", 4)) {
            handle_rel(log, cnt);
        }
    }
    fin.close();
}

int main(int argc, const char *argv[])
{
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <reloc file> <dlog file>" << endl;
        return 1;
    }

    int fd_rel;
    Elf64_Ehdr ehdr;
    if ((fd_rel = open(argv[1], O_RDONLY, 0)) < 0)
        err(EXIT_FAILURE, "open reloc \"%s\" failed", argv[1]);

    read_ehdr(fd_rel, &ehdr);
    read_symtab(fd_rel, &ehdr, argv[1]);

    sym_to_addr.push_back(0);
    read_dlog(argv[2]);

    relstrsz = strlen("rel[000000] 0066: d0 27 f4 51 00 00 00 00\0");
    // TODO: verifying the relocation
    // verify_reloc(argv[2]);

    close(fd_rel);

    return 0;
}
