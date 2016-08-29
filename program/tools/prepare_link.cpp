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

#include <stdbool.h>
#include <assert.h>

// #define ALIGN_SIZE 32
#define DEBUG 0
#if DEBUG
#define dlog(...) fprintf(stderr, __VA_ARGS__)
#else
#define dlog(...)
#endif

static inline Elf64_Addr align_addr(Elf64_Addr value, Elf64_Addr align)
{
    return (value / align) * align;
}

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

//---------- read all sections (and symtab, reltab, strtab) --------->
void read_sections(const char *file, const int& fd, const Elf64_Ehdr *ehdr)
{
    Elf64_Shdr *shdr_array = NULL;
    if (ehdr->e_shentsize != sizeof (Elf64_Shdr))
        err(EXIT_FAILURE, "%s: Shdr entry size (%d)", file, __LINE__);

    shdr_array = new Elf64_Shdr[ehdr->e_shnum];
    if (!shdr_array)
        err(EXIT_FAILURE, "%s: Shdr new (%d)", file, __LINE__);
    if (pread(fd, shdr_array, ehdr->e_shnum * sizeof(Elf64_Ehdr), ehdr->e_shoff)
            != ehdr->e_shnum * sizeof(Elf64_Ehdr))
        err(EXIT_FAILURE, "%s: Shdr pread (%d)", file, __LINE__);

    int size;
    char *shstrtab;

    size = shdr_array[ehdr->e_shstrndx].sh_size;
    shstrtab = new char[size];
    if (pread(fd, shstrtab, size, shdr_array[ehdr->e_shstrndx].sh_offset) != size)
        err(EXIT_FAILURE, "%s: Shdr shstrtab (%d)", file, __LINE__);

    for (size_t i = 0; i < ehdr->e_shnum; ++i) {
        /* This might be a .text section */
        if (shdr_array[i].sh_type == SHT_RELA
                && (shdr_array[shdr_array[i].sh_info].sh_flags & SHF_EXECINSTR)
                && shdr_array[shdr_array[i].sh_info].sh_size
                && shdr_array[shdr_array[i].sh_info].sh_type == SHT_PROGBITS) {

            unsigned info = shdr_array[i].sh_info;

            /* read the relocation table */
            size_t reltab_size = shdr_array[i].sh_size / sizeof(Elf64_Rela);
            Elf64_Rela* reltab = new Elf64_Rela[reltab_size];
            if (!reltab)
                err(EXIT_FAILURE, "%s: Shdr reltab new (%d)", file, __LINE__);
            if (pread(fd, reltab, shdr_array[i].sh_size, shdr_array[i].sh_offset)
                    != shdr_array[i].sh_size)
                err(EXIT_FAILURE, "%s: Shdr read reltab (%d)", file, __LINE__);

            /* read the symbol table */
            unsigned symtab_shndx = shdr_array[i].sh_link;
            size_t symtab_size = shdr_array[symtab_shndx].sh_size / sizeof(Elf64_Sym);
            Elf64_Sym* symtab = new Elf64_Sym[symtab_size];
            if (!symtab)
                err(EXIT_FAILURE, "%s: Shdr symtab new (%d)", file, __LINE__);
            if (pread(fd, symtab, shdr_array[symtab_shndx].sh_size,
                        shdr_array[symtab_shndx].sh_offset)
                    != shdr_array[symtab_shndx].sh_size)
                err(EXIT_FAILURE, "%s: Shdr symtab %s read (%d)", file,
                        &shstrtab[shdr_array[symtab_shndx].sh_name], __LINE__);

            /* mapping between st_value to symtab */
            std::vector<bool> removed(symtab_size, true);
            map<Elf64_Addr, unsigned> v_sym;
            for (unsigned j = 0; j < symtab_size; ++j) {
                if (symtab[j].st_shndx == info) {
                    unsigned char b  = ELF64_ST_BIND(symtab[j].st_info);
                    if (b == STB_GLOBAL) {
                        if(v_sym.find(symtab[j].st_value) != v_sym.end())
                            dlog("Warning: symtab[%u].st_value = %lx already exists\n", j,
                                    symtab[j].st_value);
                        if(symtab[j].st_value % shdr_array[info].sh_addralign)
                            dlog("Warning: symtab[%u].st_value = %lx (not aligned)\n", j,
                                    symtab[j].st_value);
                        v_sym[symtab[j].st_value] = j;
                        removed[j] = false;
                    }
                }
            }
            for (unsigned j = 0; j < symtab_size; ++j) {
                if (symtab[j].st_shndx == info) {
                    if (!(symtab[j].st_value % shdr_array[info].sh_addralign)
                            && v_sym.find(symtab[j].st_value) == v_sym.end()) {
                        v_sym[symtab[j].st_value] = j;
                        removed[j] = false;
                    }
                }
            }

            /* update relocation table */
            for (size_t j = 0; j < reltab_size; ++j) {
                Elf64_Word sym_ndx = ELF64_R_SYM(reltab[j].r_info);
                Elf64_Word r_type = ELF64_R_TYPE(reltab[j].r_info);

                // the source must be in .text section
                //  --> to remove not used basic blocks
                if (symtab[sym_ndx].st_shndx == info
                        && (symtab[sym_ndx].st_value % shdr_array[info].sh_addralign)) {
                    Elf64_Addr new_sym_location = align_addr(symtab[sym_ndx].st_value,
                            shdr_array[info].sh_addralign);

                    // search the new symbol from symtab
                    map<Elf64_Addr, unsigned>::iterator it = v_sym.find(new_sym_location);
                    if (it == v_sym.end()) {
                        dlog("Warning: There is no symbol at %lx\n", new_sym_location);
                        removed[sym_ndx] = false;
                        continue;
                    }

                    // update source and addend
                    Elf64_Word new_sym
                        = ((size_t)(it->second) - (size_t)symtab) / sizeof(Elf64_Sym);
                    reltab[j].r_info = ELF64_R_INFO(new_sym, r_type);
                    reltab[j].r_addend -= (symtab[sym_ndx].st_value - new_sym_location);
                }
            }

            /* check symbols will be removed */
            for (unsigned j = 0;j < removed.size();++j) {
                if (removed[j]) {
                    // update type --> make unused basic block as file type
                    symtab[j].st_info = ELF64_ST_INFO(STB_WEAK, STT_NOTYPE);
                }
            }

            if (pwrite(fd, symtab, shdr_array[symtab_shndx].sh_size,
                        shdr_array[symtab_shndx].sh_offset)
                    != shdr_array[symtab_shndx].sh_size)
                err(EXIT_FAILURE, "%s: Shdr symtab %s write (%d)", file,
                        &shstrtab[shdr_array[symtab_shndx].sh_name], __LINE__);

            if (pwrite(fd, reltab, shdr_array[i].sh_size, shdr_array[i].sh_offset)
                    != shdr_array[i].sh_size)
                err(EXIT_FAILURE, "%s: Shdr read reltab (%d)", file, __LINE__);

            delete symtab;
            delete reltab;
        }
    }

    delete shdr_array;
    delete shstrtab;
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
    if ((fd = open(file, O_RDWR, 0)) < 0)
        err(EXIT_FAILURE, "open \"%s\" failed (%d)", file, __LINE__);

    dlog("Run on %s:\n", file);
    read_ehdr(fd, &ehdr);
    read_sections(file, fd, &ehdr);

    close(fd);
}
//---------- read all sections (and symtab, reltab, strtab) ---------<

std::map<string, string> shlink;
std::map<string, string> shinfo;
std::map<string, Elf64_Shdr> shdr;
void read_memobj_info(int fd, Elf64_Ehdr *ehdr,
        std::map<string, std::vector<string> >& ftom, /* file to mem obj */
        std::map<string, string>& mtof, /* mem obj to file */
        const string& file)
{
    Elf64_Shdr *shdr_array = NULL;
    if (ehdr->e_shentsize != sizeof (Elf64_Shdr))
        err(EXIT_FAILURE, "Shdr entry size");

    shdr_array = new Elf64_Shdr[ehdr->e_shnum];
    if (!shdr_array)
        err(EXIT_FAILURE, "Shdr new");
    if (pread(fd, shdr_array, ehdr->e_shnum * sizeof(Elf64_Ehdr), ehdr->e_shoff)
            != ehdr->e_shnum * sizeof(Elf64_Ehdr))
        err(EXIT_FAILURE, "Shdr pread");

    char *shstrtab;
    size_t size = shdr_array[ehdr->e_shstrndx].sh_size;
    shstrtab = new char[size];
    if (pread(fd, shstrtab, size, shdr_array[ehdr->e_shstrndx].sh_offset) != size)
        err(EXIT_FAILURE, "Shdr shstrtab");

    for (int i = 0; i < ehdr->e_shnum; ++i) {
        std::map<string, Elf64_Shdr>::iterator it_shdr
            = shdr.find(&shstrtab[shdr_array[i].sh_name]);
        if (it_shdr == shdr.end()) {
            Elf64_Word tmp = shdr_array[i].sh_size;
            shdr_array[i].sh_size = 0;
            shdr[&shstrtab[shdr_array[i].sh_name]] = shdr_array[i];
            shdr_array[i].sh_size = tmp;
        }

        if (shdr_array[i].sh_type == SHT_SYMTAB) {
            if (!shdr_array[i].sh_size) continue;

            /* read the linked string table */
            size = shdr_array[shdr_array[i].sh_link].sh_size;
            char *strtab = new char[size];
            if (!strtab)
                err(EXIT_FAILURE, "Shdr symtab strtab new");
            if (pread(fd, strtab, size, shdr_array[shdr_array[i].sh_link].sh_offset) != size)
                err(EXIT_FAILURE, "Shdr symtab strtab");

            /* read the symbol table */
            size = shdr_array[i].sh_size;
            Elf64_Sym *sym = (Elf64_Sym *)malloc(size);
            if (!sym)
                err(EXIT_FAILURE, "Shdr symtab malloc");
            if (pread(fd, sym, size, shdr_array[i].sh_offset) != size)
                err(EXIT_FAILURE, "Shdr symtab");

            for (int j = 0; j < size/sizeof(Elf64_Sym); ++j) {
                unsigned char type = ELF64_ST_TYPE(sym[j].st_info);
                unsigned char bind = ELF64_ST_BIND(sym[j].st_info);
                string sym_name(&strtab[sym[j].st_name]);
                if (type != STT_FILE && sym[j].st_shndx != SHN_UNDEF
                        && (bind == STB_GLOBAL || bind == STB_WEAK)) {
                    if (bind == STB_GLOBAL)
                        mtof[sym_name] = file;
                    else {
                        std::map<string, string>::iterator it = mtof.find(sym_name);
                        if (it == mtof.end())
                            mtof[sym_name] = file;
                    }
                }
                if (type == STT_NOTYPE && bind == STB_GLOBAL
                        && sym[j].st_shndx == SHN_UNDEF)
                    ftom[file].push_back(sym_name);
            }

            free(sym);
            delete strtab;
        }

        /* to check the sh_link */
        if (shdr_array[i].sh_type == SHT_SYMTAB
                || shdr_array[i].sh_type == SHT_RELA
                || shdr_array[i].sh_type == SHT_REL) {
            std::map<string, string>::iterator it_link
                = shlink.find(&shstrtab[shdr_array[i].sh_name]);
            int link = shdr_array[i].sh_link;
            if (it_link == shlink.end())
                shlink[&shstrtab[shdr_array[i].sh_name]]
                    = &shstrtab[shdr_array[link].sh_name];
            else {
                if ((it_link->second).compare(&shstrtab[shdr_array[link].sh_name])) {
                    cout << "sh_link weired:" << endl;
                    cout << it_link->first << " => " << it_link->second;
                    cout << " not " << &shstrtab[shdr_array[link].sh_name] << endl << endl;
                }
            }
        }

        /* to check the sh_info */
        if (shdr_array[i].sh_type == SHT_RELA
                || shdr_array[i].sh_type == SHT_REL) {
            std::map<string, string>::iterator it_info
                = shinfo.find(&shstrtab[shdr_array[i].sh_name]);
            int info = shdr_array[i].sh_info;
            if (it_info == shinfo.end())
                shinfo[&shstrtab[shdr_array[i].sh_name]]
                    = &shstrtab[shdr_array[info].sh_name];
            else {
                if ((it_info->second).compare(&shstrtab[shdr_array[info].sh_name])) {
                    cout << "sh_info weired:" << endl;
                    cout << it_info->first << " => " << it_info->second;
                    cout << " not " << &shstrtab[shdr_array[info].sh_name] << endl << endl;
                }
            }
        }
    }

    delete shstrtab;
    delete shdr_array;
}

void find_needed_relocatables(const std::map<string, std::vector<string> >& ftom,
        const std::map<string, string>& mtof,
        std::set<string>& needed,   /* all needed mem obj */
        std::set<string>& ret)      /* all needed file */
{
    std::set<string> r = needed;    /* current needed mem obj */
    while (!r.empty()) {
        string obj = *r.begin();
        r.erase(r.begin());

        std::map<string, string>::const_iterator ifile = mtof.find(obj);
        if (ifile == mtof.end())
            cout << "weired: " << obj << endl;
        std::set<string>::iterator it = ret.find(ifile->second);
        if (it != ret.end())
            continue;
        ret.insert(ifile->second);

        std::map<string, vector<string> >::const_iterator imobj = ftom.find(ifile->second);
        if (imobj == ftom.end())
            continue;
        for (int i = 0; i < (imobj->second).size(); ++i) {
            if (needed.find((imobj->second)[i]) == needed.end()) {
                r.insert((imobj->second)[i]);
                needed.insert((imobj->second)[i]);
            }
        }
    }
}

#define ENTRY_POINT "_start\0"
void select_needed_relocatables(const std::vector<string>& files, std::set<string>& ret)
{
    std::map<string, std::vector<string> > ftom;
    std::map<string, string> mtof;

    for (int i = 0; i < files.size(); ++i) {
        int fd;
        Elf64_Ehdr ehdr;
        if ((fd = open(files[i].c_str(), O_RDONLY, 0)) < 0)
            err(EXIT_FAILURE, "open \"%s\" failed", files[i].c_str());

        read_ehdr(fd, &ehdr);

        std::vector<string> v;
        ftom[files[i]] = v;
        read_memobj_info(fd, &ehdr, ftom, mtof, files[i]);

        close(fd);
    }

    std::set<string> needed;
    needed.insert(ENTRY_POINT);
    find_needed_relocatables(ftom, mtof, needed, ret);
}

int main(int argc, const char *argv[])
{
    std::vector<string> v;
    string name;
    while (cin >> name) {
        v.push_back(name);
    }
    std::set<string> relocatables;
    select_needed_relocatables(v, relocatables);
    v.clear();

    for (std::set<string>::iterator it = relocatables.begin();
            it != relocatables.end(); ++it) {
        read_relocatable(it->c_str());
    }
    return 0;
}
