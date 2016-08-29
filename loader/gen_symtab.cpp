#define loff_t off_t

#include <err.h>
#include <fcntl.h>
#include <elf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <string.h>
#include <time.h>

#include <endian.h>
#if BYTE_ORDER == BIG_ENDIAN
# define byteorder ELFDATA2MSB
#elif BYTE_ORDER == LITTLE_ENDIAN
# define byteorder ELFDATA2LSB
#else
# error "Unknown BYTE_ORDER " BYTE_ORDER
# define byteorder ELFDATANONE
#endif

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

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <vector>
using namespace std;

size_t n_symtab;
Elf64_Sym* symtab;
char *strtab;
size_t n_reltab;
Elf64_Rela* reltab;

//---------- read all sections (and symtab, reltab, strtab) --------->
void read_sections(const int& fd, const Elf64_Ehdr *ehdr)
{
    Elf64_Shdr *shdr_array = NULL;
    if (ehdr->e_shentsize != sizeof (Elf64_Shdr))
        err(EXIT_FAILURE, "Shdr entry size");

    /* read section headers */
    shdr_array = new Elf64_Shdr[ehdr->e_shnum];
    if (!shdr_array)
        err(EXIT_FAILURE, "Shdr new");
    if (pread(fd, shdr_array, ehdr->e_shnum * sizeof(Elf64_Ehdr), ehdr->e_shoff)
            != ehdr->e_shnum * sizeof(Elf64_Ehdr))
        err(EXIT_FAILURE, "Shdr pread");

    /* read section header strings */
    char *shstrtab;
    size_t shsize = shdr_array[ehdr->e_shstrndx].sh_size;
    shstrtab = new char[shsize];
    if (pread(fd, shstrtab, shsize, shdr_array[ehdr->e_shstrndx].sh_offset) != shsize)
        err(EXIT_FAILURE, "Shdr shstrtab");

    /* find symbol table, string table, rel table */
    size_t symtab_ndx = 0;
    size_t strtab_ndx = 0;
    size_t reltab_ndx = 0;
    for (size_t i = 0; i < ehdr->e_shnum; ++i) {
        if (shdr_array[i].sh_type == SHT_SYMTAB) {
            symtab_ndx = i;
            strtab_ndx = shdr_array[i].sh_link;
        } else if (shdr_array[i].sh_type == SHT_RELA
                && (shdr_array[shdr_array[i].sh_info].sh_flags & SHF_EXECINSTR)) {
            reltab_ndx = i;
        }
    }

    /* read symtab, strtab, reltab */
    n_symtab = shdr_array[symtab_ndx].sh_size / sizeof(Elf64_Sym);
    symtab = new Elf64_Sym[n_symtab];
    if (!symtab)
        err(EXIT_FAILURE, "Shdr symtab %s new", &shstrtab[shdr_array[symtab_ndx].sh_name]);
    if (pread(fd, symtab, shdr_array[symtab_ndx].sh_size, shdr_array[symtab_ndx].sh_offset)
            != shdr_array[symtab_ndx].sh_size)
        err(EXIT_FAILURE, "Shdr symtab %s pread", &shstrtab[shdr_array[symtab_ndx].sh_name]);

    strtab = new char[shdr_array[strtab_ndx].sh_size];
    if (!strtab)
        err(EXIT_FAILURE, "Shdr strtab new");
    if (pread(fd, strtab, shdr_array[strtab_ndx].sh_size, shdr_array[strtab_ndx].sh_offset)
            != shdr_array[strtab_ndx].sh_size)
        err(EXIT_FAILURE, "Shdr strtab pread");

    n_reltab = shdr_array[reltab_ndx].sh_size / sizeof(Elf64_Rela);
    reltab = new Elf64_Rela[n_reltab];
    if (!reltab)
        err(EXIT_FAILURE, "Shdr reltab %s new", &shstrtab[shdr_array[reltab_ndx].sh_name]);
    if (pread(fd, reltab, shdr_array[reltab_ndx].sh_size, shdr_array[reltab_ndx].sh_offset)
            != shdr_array[reltab_ndx].sh_size)
        err(EXIT_FAILURE, "Shdr reltab %s pread", &shstrtab[shdr_array[reltab_ndx].sh_name]);


    /* find timestamp functions */
    for (size_t i = 1; i < n_symtab; ++i) {
        if (symtab[i].st_shndx < ehdr->e_shnum
                && shdr_array[symtab[i].st_shndx].sh_flags & SHF_EXECINSTR)
            printf("%lu %lx\n", i, symtab[i].st_value);
    }
}

/*
 * read_relocatable()
 * 1. read elf header
 * 2. read sections
 * 3. read symtab (assuming that each mem obj is corresponding to a symbol)
 * 4. read reltab
 */
void read_relocatable(const char* file)
{
    int fd;
    Elf64_Ehdr ehdr;
    if ((fd = open(file, O_RDONLY, 0)) < 0)
        err(EXIT_FAILURE, "open \"%s\" failed", file);

    read_ehdr(fd, &ehdr);
    read_sections(fd, &ehdr);

    close(fd);
}
//---------- read all sections (and symtab, reltab, strtab) ---------<

int main(int argc, const char *argv[])
{
    read_relocatable("program");
    return 0;
}
