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

#include <assert.h>

#define ENTRY_POINT "_start\0"
const char *program_name = "target.rel\0";

static unsigned char nop[128];
static inline int align(int v, int a)
{
    if (!a)
        return v;
    int r = v % a;
    if (r)
        v = v - r + a;
    return v;
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
std::map<string, string> shlink;
std::map<string, string> shinfo;
std::map<string, Elf64_Shdr> shdr;
struct section_t {
    string name;
    Elf64_Shdr shdr;
    unsigned char* data; /* big blobs */
    int size;
    int capacity;
};
std::vector<section_t> sectionv;

/*
 * Warning: this can incur too much frequent memory allocation
 * If this can be a problem, replace it with allocating a large chunk and spliting it
 */
void push_data(section_t* st, const void* data, const int size)
{
    if (st->shdr.sh_type == SHT_NOBITS)
        return;
    if (!st->data) {
        st->data = new unsigned char[st->capacity];
        st->size = 0;
    }
    memcpy(&(st->data)[st->size], data, size);
    st->size += size;
}

section_t* get_section(const string& shname)
{
    for (int j = 0; j < sectionv.size(); ++j)
        if(!sectionv[j].name.compare(shname))
            return &sectionv[j];
    err(EXIT_FAILURE, "Shdr section is not found at %d", __LINE__);
}

struct loaded_symtab_t {
    int sh_ndx;
    Elf64_Sym *sym;
    std::vector<int> sym_ndx;
};

struct loaded_strtab_t {
    int sh_ndx;
    char* strtab;
};

struct undef_rel_t {
    section_t* reltab;
    int relndx;
    string global_sym;
};
std::vector<undef_rel_t> undef_reltab;
std::map<string, int> global_symtab;

void read_symtab(const int& fd, const Elf64_Shdr& sh, const char* shstrtab,
        loaded_symtab_t& loaded_sym, const char* strtab)
{
    /* find this symbol's section */
    section_t *symtab = get_section(&shstrtab[sh.sh_name]);

    /* read the symbol table */
    int size = sh.sh_size / sizeof(Elf64_Sym);
    Elf64_Sym* sym = new Elf64_Sym[size];
    loaded_sym.sym = sym;
    if (!sym)
        err(EXIT_FAILURE, "Shdr symtab %s new", &shstrtab[sh.sh_name]);
    if (pread(fd, sym, sh.sh_size, sh.sh_offset) != sh.sh_size)
        err(EXIT_FAILURE, "Shdr symtab %s read", &shstrtab[sh.sh_name]);

    /*
     * load the symtab to the global "symtab"
     * 1. skip the very first symbol (i.e., j = 0)
     * 2. skip the symbol which has the FILE type
     * 3. skip the symbol which has the NOTYPE type and the UND section
     *    (i.e., defined in another file)
     */
    loaded_sym.sym_ndx.push_back(-1); /* for j = 0 */
    for (int j = 1; j < size; ++j) {
        unsigned char type = ELF64_ST_TYPE(sym[j].st_info);
        unsigned char bind = ELF64_ST_BIND(sym[j].st_info);

        if (type != STT_FILE && !(type == STT_NOTYPE && sym[j].st_shndx == SHN_UNDEF)) {
            /* push to the symtab */
            push_data(symtab, &sym[j], sizeof(Elf64_Sym));

            /* for the local relocation */
            int sym_ndx = (symtab->size/sizeof(Elf64_Sym)) - 1;
            loaded_sym.sym_ndx.push_back(sym_ndx);

            /* keep global symbols for the undefined type relocation */
            if (bind == STB_GLOBAL || bind == STB_WEAK) {
                std::map<string, int>::iterator it
                    = global_symtab.find(&strtab[sym[j].st_name]);
                if (it == global_symtab.end())
                    global_symtab[&strtab[sym[j].st_name]] = sym_ndx;
                else if (bind == STB_GLOBAL)
                    it->second = sym_ndx;
            }
        } else {
            loaded_sym.sym_ndx.push_back(-1);
        }
    }
}

int read_strtab(const int& fd, const Elf64_Shdr& sh, loaded_strtab_t& loaded_str,
        const char* shstrtab)
{
    /* read the linked string table */
    loaded_str.strtab = new char[sh.sh_size];
    if (!loaded_str.strtab)
        err(EXIT_FAILURE, "Shdr symtab strtab new");
    if (pread(fd, loaded_str.strtab, sh.sh_size, sh.sh_offset) != sh.sh_size)
        err(EXIT_FAILURE, "Shdr symtab strtab");

    /* find this strtab's section */
    section_t *s = get_section(&shstrtab[sh.sh_name]);
    int ret = s->size;
    push_data(s, loaded_str.strtab, sh.sh_size);
    return ret;
}

/* update symbol's st_size */
#include <algorithm>
struct st_value_t {
    int st_value;
    int sym_ndx;    /* the symbol index in the global symtab */
};
bool comp(const st_value_t& r, const st_value_t& l) {
    return r.st_value < l.st_value;
}
void update_st_size(const Elf64_Ehdr *ehdr,
        const Elf64_Shdr *shdr_array,
        loaded_symtab_t& loaded_symtab,
        Elf64_Sym *symtab, char *strtab)
{
    /* generate shndx-to-st_value mapping */
    std::map<int, std::vector<st_value_t> > shndx_to_st_value;
    std::vector<int>& sym_ndx = loaded_symtab.sym_ndx;
    for (int i = 0; i < sym_ndx.size(); ++i) {
        int shndx = loaded_symtab.sym[i].st_shndx;
        std::map<int, std::vector<st_value_t> >::iterator it
            = shndx_to_st_value.find(shndx);
        if (it == shndx_to_st_value.end()) {
            std::vector<st_value_t> v;
            shndx_to_st_value[shndx] = v;
            it = shndx_to_st_value.find(shndx);
        }

        st_value_t tmp = {loaded_symtab.sym[i].st_value, sym_ndx[i]};
        (it->second).push_back(tmp);
    }

    /* sort each section's symbols by st_value and updat st_size in global symtab */
    for (std::map<int, std::vector<st_value_t> >::iterator it = shndx_to_st_value.begin();
            it != shndx_to_st_value.end(); ++it) {

        /* get section alignment and flags */
        Elf64_Word sh_align = it->first < ehdr->e_shnum ?
            shdr_array[(it->first)].sh_addralign : 1;
        Elf64_Word sh_flags = it->first < ehdr->e_shnum ?
            shdr_array[(it->first)].sh_flags : 0;

        /* sort */
        std::vector<st_value_t>& v = it->second;
        sort(v.begin(), v.end(), comp);

        if (sh_flags & SHF_EXECINSTR) {
            /* code object */
            int last_memobj = -1;
            char *func = NULL;
            size_t func_len = 0;
            for (int i = 0; i < v.size(); ++i) {
                assert (v[i].sym_ndx >= 0 && "Symbol index is negative!!");
                unsigned char t = ELF64_ST_TYPE(symtab[v[i].sym_ndx].st_info);
                if (t == STT_FUNC) {
                    assert(!(v[i].st_value % sh_align) && "FUNC is not aligned!!");
                    if (last_memobj >= 0) {
                        symtab[v[last_memobj].sym_ndx].st_size
                            = align(v[i].st_value - v[last_memobj].st_value, sh_align);
                        assert (symtab[v[last_memobj].sym_ndx].st_size >= 0
                                && "Symbol size is negative!!");
                    }
                    last_memobj = i;
                    func = &strtab[symtab[v[i].sym_ndx].st_name];
                    func_len = strlen(func);
                } else if (!strncmp(&strtab[symtab[v[i].sym_ndx].st_name], func,
                            func_len)) { // this symbol's name contains the func's name
                    symtab[v[last_memobj].sym_ndx].st_size
                        = align(v[i].st_value - v[last_memobj].st_value, sh_align);
                    assert (symtab[v[last_memobj].sym_ndx].st_size >= 0
                            && "Symbol size is negative!!");
                    last_memobj = i;
                }
            }
            if ((sh_flags & SHF_EXECINSTR) && func) {
                assert (v[last_memobj].sym_ndx >= 0 && "Symbol index is negative!!");
                symtab[v[last_memobj].sym_ndx].st_size
                    = shdr_array[it->first].sh_size - v[last_memobj].st_value;
                assert (symtab[v[last_memobj].sym_ndx].st_size >= 0
                        && "Symbol size is negative!!");
            }
        } else {
            for (int i = 0; i < v.size()-1; ++i) {
                if (v[i].sym_ndx >= 0 && !symtab[v[i].sym_ndx].st_size)
                    symtab[v[i].sym_ndx].st_size = v[i+1].st_value - v[i].st_value;
            }
            /* for the very last memory object */
            if (v.size() && v[v.size()-1].sym_ndx >= 0
                    && !symtab[v[v.size()-1].sym_ndx].st_size)
                symtab[v[v.size()-1].sym_ndx].st_size
                    = shdr_array[it->first].sh_size - v[v.size()-1].st_value;
        }

//        unsigned last_memobj = 0;
//        unsigned func_size = 0;
//        for (int i = 0; i < v.size()-1; ++i) {
//            unsigned char t = ELF64_ST_TYPE(symtab[v[i].sym_ndx].st_info);
//
//            /* code object is not aligned and not a FUNC --> Treat it as WEAK bind */
//            if ((sh_flags & SHF_EXECINSTR) && (v[i].st_value % sh_align)
//                    && t != STT_FUNC) {
//                if (v[last_memobj].st_value + symtab[v[last_memobj].sym_ndx].st_size
//                        <= v[i].st_value) {
//                    printf("Warning: v[%d].st_value = %u at %u\n", i, v[i].st_value, __LINE__);
//                    symtab[v[last_memobj].sym_ndx].st_size
//                        = align(v[i].st_value - v[last_memobj].st_value, sh_align);
//                }
//                continue;
//            }
//
//            /*
//             * Update st_size:
//             * 1) the size is zero or
//             * 2) the size of FUNC is too large
//             */
//            if ((v[i].sym_ndx >= 0 && !symtab[v[i].sym_ndx].st_size)
//                    ||
//                    /* FUNC is too larger */
//                    (v[i].sym_ndx >= 0 && t == STT_FUNC
//                     && symtab[v[i].sym_ndx].st_size + v[i].st_value > v[i+1].st_value))
//            {
//                if (t == STT_FUNC)     func_size = symtab[v[i].sym_ndx].st_size;
//                else                   func_size = 0;
//
//                symtab[v[i].sym_ndx].st_size = v[i+1].st_value - v[i].st_value;
//
//                /*
//                 * The minimum value of st_size should be sh_align.
//                 * Currently, only apply it to code object.
//                 */
//                if (sh_flags & SHF_EXECINSTR) {
//                    if (symtab[v[i].sym_ndx].st_size
//                            && (symtab[v[i].sym_ndx].st_size % sh_align))
//                        symtab[v[i].sym_ndx].st_size
//                            = align(symtab[v[i].sym_ndx].st_size, sh_align);
//                    last_memobj = i;
//                }
//            }
//        }
//        /* for the very last memory object */
//        if (v.size() && v[v.size()-1].sym_ndx >= 0 &&
//                !symtab[v[v.size()-1].sym_ndx].st_size && !(v[v.size()-1].st_value % sh_align))
//            symtab[v[v.size()-1].sym_ndx].st_size
//                = shdr_array[it->first].sh_size - v[v.size()-1].st_value;
//        else if (func_size)
//            symtab[v[last_memobj].sym_ndx].st_size = func_size;
    }
}

void read_sections(const int& fd, const Elf64_Ehdr *ehdr)
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

    int size;
    char *shstrtab;

    size = shdr_array[ehdr->e_shstrndx].sh_size;
    shstrtab = new char[size];
    if (pread(fd, shstrtab, size, shdr_array[ehdr->e_shstrndx].sh_offset) != size)
        err(EXIT_FAILURE, "Shdr shstrtab");

    /* load symbols */
    std::vector<loaded_symtab_t> loaded_symtab;
    std::vector<loaded_strtab_t> loaded_strtab;
    int loading_shndx[ehdr->e_shnum];
    section_t* shndx_to_section[ehdr->e_shnum];
    int strtab_base[ehdr->e_shnum];
    for (int i = 0; i < ehdr->e_shnum; ++i) {
        strtab_base[i] = -1;
        loading_shndx[i] = 0;
    }

    for (int i = 0; i < ehdr->e_shnum; ++i) {
        /* cache the shndx to section_t* */
        shndx_to_section[i] = get_section(&shstrtab[shdr_array[i].sh_name]);

        if (shdr_array[i].sh_type == SHT_SYMTAB) {
            if (!shdr_array[i].sh_size)
                continue;

            /* search it among loaded symtabs */
            bool found = false;
            for (int j = 0; j < loaded_symtab.size(); ++j) {
                if (loaded_symtab[j].sh_ndx == i) {
                    found = true;
                    break;
                }
            }
            if (found)
                continue;

            /* search its linked strtab among loaded strtabs */
            char* strtab = NULL;
            for (int j = 0; j < loaded_strtab.size(); ++j) {
                if (loaded_strtab[j].sh_ndx == shdr_array[i].sh_link) {
                    strtab = loaded_strtab[j].strtab;
                    break;
                }
            }
            if (!strtab) {
                loaded_strtab_t tmp2 = {shdr_array[i].sh_link, NULL};
                strtab_base[shdr_array[i].sh_link]
                    = read_strtab(fd, shdr_array[shdr_array[i].sh_link], tmp2, shstrtab);
                loaded_strtab.push_back(tmp2);
                strtab = tmp2.strtab;
            }

            /* load and save this symtab */
            loaded_symtab_t tmp;
            tmp.sh_ndx = i;
            read_symtab(fd, shdr_array[i], shstrtab, tmp, strtab);
            loaded_symtab.push_back(tmp);

            /* check which sections must be loaded */
            for (int j = 0; j < shdr_array[i].sh_size / sizeof(Elf64_Sym); ++j)
                if (tmp.sym[j].st_shndx && (tmp.sym[j].st_shndx < ehdr->e_shnum)
                        && shdr_array[tmp.sym[j].st_shndx].sh_size)
                    loading_shndx[tmp.sym[j].st_shndx] = 1;
        }
    }

    /* load sections */
    for (int i = 0; i < ehdr->e_shnum; ++i) {
        if (loading_shndx[i] && shdr_array[i].sh_type != SHT_NOBITS) {
            if (shdr_array[i].sh_type == SHT_SYMTAB
                    || shdr_array[i].sh_type == SHT_STRTAB
                    || shdr_array[i].sh_type == SHT_RELA)
                err(EXIT_FAILURE, "Shdr load section %s invalid type",
                        &shstrtab[shdr_array[i].sh_name]);

            if (!shdr_array[i].sh_size)
                err(EXIT_FAILURE, "Shdr shdr_array[%d].sh_size=0", i);

            /* find this section */
            section_t *s = shndx_to_section[i];

            /* read the section data */
            unsigned char* data = new unsigned char[shdr_array[i].sh_size];
            if (!data)
                err(EXIT_FAILURE, "Shdr section %s data new",
                        &shstrtab[shdr_array[i].sh_name]);
            if (pread(fd, data, shdr_array[i].sh_size, shdr_array[i].sh_offset)
                    != shdr_array[i].sh_size)
                err(EXIT_FAILURE, "Shdr read %d", i);

            /* append the data of shdr_array[i] to the global section s */
            int tmp = align(s->size, shdr_array[i].sh_addralign);
            push_data(s, nop, tmp - s->size);
            shdr_array[i].sh_addr = s->size;
            push_data(s, data, shdr_array[i].sh_size);

            delete data;
        }
    }

    /* for the global symbol table, update st_size, st_name, st_value, st_shndx */
    for (int i = 0; i < loaded_symtab.size(); ++i) {
        if (!shdr_array[loaded_symtab[i].sh_ndx].sh_size)
            err(EXIT_FAILURE, "Shdr loaded_symtab[%d].sh_size=0", loaded_symtab[i].sh_ndx);

        section_t *s = shndx_to_section[loaded_symtab[i].sh_ndx];
        int link = shdr_array[loaded_symtab[i].sh_ndx].sh_link;

        /* update st_size */
        int str_ndx = -1;
        for (int j = 0; j < loaded_strtab.size(); ++j) {
            if (shdr_array[loaded_symtab[i].sh_ndx].sh_link == loaded_strtab[j].sh_ndx) {
                str_ndx = j;
                break;
            }
        }
        assert(str_ndx >= 0 && "There is no corresponding strtab!!");
        update_st_size(ehdr, shdr_array, loaded_symtab[i], (Elf64_Sym *)(s->data),
                loaded_strtab[str_ndx].strtab);

        std::vector<int>& sym_ndx = loaded_symtab[i].sym_ndx;
        for (int j = 0; j < sym_ndx.size(); ++j) {
            if (sym_ndx[j] == -1)
                continue;

            Elf64_Sym *sym = &((Elf64_Sym *)(s->data))[sym_ndx[j]];

            /* update symbol's st_name: COM & ABS also need it */
            if (strtab_base[link] == -1)
                err(EXIT_FAILURE, "Shdr strtab_base[%d] is -1", link);
            sym->st_name += strtab_base[link];

            /* skip COM & ABS */
            if (!sym->st_shndx || (sym->st_shndx >= ehdr->e_shnum))
                continue;

            /* update symbol's st_shndx, st_value */
            const int st_shndx = sym->st_shndx;
            sym->st_shndx = ((unsigned long)shndx_to_section[st_shndx]
                    - (unsigned long)sectionv.data()) / sizeof(section_t);
            sym->st_value += shdr_array[st_shndx].sh_addr;
        }
    }

    for (int i = 0; i < ehdr->e_shnum; ++i) {
        /* x86_64 ABI only allows SHT_RELA i.e., no SHT_REL */
        if (shdr_array[i].sh_type == SHT_RELA) {
            if (!shdr_array[i].sh_size)
                continue;

            /* get the global relocation table section */
            section_t* reltab = shndx_to_section[i];

            Elf64_Addr base = shdr_array[shdr_array[i].sh_info].sh_addr;

            /* get linked symbol table */
            int link = shdr_array[i].sh_link;
            if (shdr_array[link].sh_type != SHT_SYMTAB)
                err(EXIT_FAILURE, "Shdr %d is not symtab", link);

            /* check the symtab is loaded */
            int sym_ndx = -1;
            for (int j = 0; j < loaded_symtab.size(); ++j) {
                if (link == loaded_symtab[j].sh_ndx) {
                    sym_ndx = j;
                    break;
                }
            }
            if (sym_ndx == -1)
                err(EXIT_FAILURE, "Shdr %d is not loaded", link);

            /* check the strtab is loaded */
            int str_ndx = -1;
            for (int j = 0; j < loaded_strtab.size(); ++j) {
                if (shdr_array[link].sh_link == loaded_strtab[j].sh_ndx) {
                    str_ndx = j;
                    break;
                }
            }
            if (str_ndx == -1)
                err(EXIT_FAILURE, "Shdr strtab %d is not loaded",
                        shdr_array[link].sh_link);

            /* read the relocation table */
            size = shdr_array[i].sh_size / sizeof(Elf64_Rela);
            Elf64_Rela* rel = new Elf64_Rela[size];
            if (!rel)
                err(EXIT_FAILURE, "Shdr reltab new");
            if (pread(fd, rel, shdr_array[i].sh_size, shdr_array[i].sh_offset)
                    != shdr_array[i].sh_size)
                err(EXIT_FAILURE, "Shdr read reltab");

            /* update the information of the relocation */
            for (int j = 0; j < size; ++j) {
                Elf64_Xword r_sym = ELF64_R_SYM(rel[j].r_info);
                Elf64_Xword r_type = ELF64_R_TYPE(rel[j].r_info);

                rel[j].r_info = ELF64_R_INFO(loaded_symtab[sym_ndx].sym_ndx[r_sym], r_type);
                rel[j].r_offset += base;

                int rel_ndx = reltab->size / sizeof(Elf64_Rela);
                push_data(reltab, &rel[j], sizeof(Elf64_Rela));

                if (loaded_symtab[sym_ndx].sym_ndx[r_sym] == -1) {
                    Elf64_Sym* sym = &(loaded_symtab[sym_ndx].sym[r_sym]);
                    char* sym_str = &(loaded_strtab[str_ndx].strtab[sym->st_name]);
                    undef_rel_t tmp = {reltab, rel_ndx, sym_str};
                    undef_reltab.push_back(tmp);
                }
            }

            delete rel;
            /*
            if (R_X86_64_64) {
            }
            if (R_X86_64_PC32) {
            }
            */
        }
    }

    for (int i = 0; i < loaded_symtab.size(); ++i) {
        delete loaded_symtab[i].sym;
    }
    for (int i = 0; i < loaded_strtab.size(); ++i) {
        delete loaded_strtab[i].strtab;
    }

    delete shstrtab;
    delete shdr_array;
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

void update_section_capacity(const int fd, const Elf64_Ehdr* ehdr)
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

    int size;
    char *shstrtab;

    size = shdr_array[ehdr->e_shstrndx].sh_size;
    shstrtab = new char[size];
    if (pread(fd, shstrtab, size, shdr_array[ehdr->e_shstrndx].sh_offset) != size)
        err(EXIT_FAILURE, "Shdr shstrtab");

    for (int i = 0; i < ehdr->e_shnum; ++i) {
        section_t* s = get_section(&shstrtab[shdr_array[i].sh_name]);
        s->capacity = align(s->capacity, shdr_array[i].sh_addralign);
        s->capacity += shdr_array[i].sh_size;
    }

    delete shstrtab;
    delete shdr_array;
}

void update_capacity(const char* file) {
    int fd;
    Elf64_Ehdr ehdr;
    if ((fd = open(file, O_RDONLY, 0)) < 0)
        err(EXIT_FAILURE, "open \"%s\" failed", file);

    read_ehdr(fd, &ehdr);
    update_section_capacity(fd, &ehdr);

    close(fd);
}

void update_undef_rel(void)
{
    for (int i = 0; i < sectionv.size(); ++i) {
        for (int j = 0; j < undef_reltab.size(); ++j) {
            /* assumption: only one symtab exists */
            int symndx = global_symtab[undef_reltab[j].global_sym];
            Elf64_Rela* rel = &((Elf64_Rela*)
                    ((undef_reltab[j].reltab)->data))[undef_reltab[j].relndx];

            Elf64_Xword r_sym = symndx;
            Elf64_Xword r_type = ELF64_R_TYPE(rel->r_info);
            rel->r_info = ELF64_R_INFO(r_sym, r_type);
        }
    }
}

/*
 * process of sorting symbols
 * 1. sort symbols by {st_shndx, st_value, st_size}
 *    while managing "initial symndx to old symndx" mapping
 * 2. update the relocation table (r_info's r_sym)
 *    according to the "initial symndx to old symndx" mapping
 */
bool comp_sym(const Elf64_Sym *r, const Elf64_Sym *l)
{
    if (r->st_shndx != l->st_shndx)
        return r->st_shndx < l->st_shndx;
    else if (r->st_shndx < sectionv.size() &&
            sectionv[r->st_shndx].shdr.sh_type == SHT_NOBITS)
        return r->st_size > l->st_size;
    else if (r->st_value != l->st_value)
        return r->st_value < l->st_value;
    else
        return r->st_size > l->st_size;
}

static Elf64_Sym *main_sym = NULL;

void sort_sym(void)
{
    section_t *symtab = get_section(".symtab\0");
    size_t n_symtab = symtab->size / sizeof(Elf64_Sym);
    std::vector<Elf64_Sym *> v;
    Elf64_Sym *sym = (Elf64_Sym *)(symtab->data);
    for (int i = 0; i < n_symtab; ++i) {
        v.push_back(&sym[i]);
    }
    sort(v.begin(), v.end(), comp_sym);

    /* generate "initial symndx to new symndx" */
    int sym_ndx[n_symtab];
    for (int i = 0; i < n_symtab; ++i) {
        int tmp = ((unsigned long)v[i] - (unsigned long)sym) / sizeof(Elf64_Sym);
        sym_ndx[tmp] = i;
    }

    /* update the r_info's r_sym --> point new symtab */
    for (int i = 0; i < sectionv.size(); ++i) {
        if (sectionv[i].shdr.sh_type == SHT_RELA) {
            Elf64_Rela *reltab = (Elf64_Rela *)sectionv[i].data;
            int n_reltab = sectionv[i].size / sizeof(Elf64_Rela);
            Elf64_Sym *sym = (Elf64_Sym *)sectionv[sectionv[i].shdr.sh_link].data;

            for (int j = 0; j < n_reltab; ++j) {
                const Elf64_Xword r_sym
                    = sym_ndx[(unsigned int)ELF64_R_SYM(reltab[j].r_info)];
                const Elf64_Xword r_type = ELF64_R_TYPE(reltab[j].r_info);
                reltab[j].r_info = ELF64_R_INFO(r_sym, r_type);
            }
        }
    }

    /* apply the sorted symtab */
    Elf64_Sym *data = (Elf64_Sym *)(new unsigned char[symtab->size]);
    for (int i = 0; i < n_symtab; ++i) {
        data[i] = *v[i];
    }
    delete sym;
    symtab->data = (unsigned char *)data;
    (*(Elf64_Sym *)data).st_size = 0;

    /* find the main */
    int main_symndx = sym_ndx[global_symtab[ENTRY_POINT]];
    main_sym = &(data[main_symndx]);
}

//-------- generate a linked relocatable file -------->
void write_ehdr(const int fd, Elf64_Ehdr& ehdr)
{
    static const unsigned char e_ident[EI_NIDENT] =
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
    memcpy(&ehdr.e_ident, e_ident, sizeof(unsigned char)*EI_NIDENT);

    ehdr.e_version = EV_CURRENT;
    ehdr.e_type = ET_REL;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_shentsize = sizeof(Elf64_Shdr);

    /* it seems that the e_flags is 0 */
    ehdr.e_flags = 0;

    /* e_shstrndx will be given in addition to e_shnum, e_entry, e_shoff */
    if (!ehdr.e_shnum)
        err(EXIT_FAILURE, "write ehdr no entry");
    if (!ehdr.e_entry)
        err(EXIT_FAILURE, "write ehdr no entry");
    if (!ehdr.e_shoff)
        err(EXIT_FAILURE, "write ehdr no shoff");

    if (ehdr.e_phoff || ehdr.e_phentsize || ehdr.e_phnum)
        err(EXIT_FAILURE, "write ehdr ph exists");

    write(fd, (char*)&ehdr, sizeof(Elf64_Ehdr));
}

void gen_relocatable(void)
{
    /* find .shstrtab, .strtab and .symtab sections */
    section_t* p_shstrtab = NULL;
    section_t* p_strtab = NULL;
    section_t* p_symtab = NULL;
    for (int i = 0; i < sectionv.size(); ++i) {
        if (!sectionv[i].name.compare(".shstrtab\0"))
            p_shstrtab = &sectionv[i];
        if (!sectionv[i].name.compare(".strtab\0"))
            p_strtab = &sectionv[i];
        if (!sectionv[i].name.compare(".symtab\0"))
            p_symtab = &sectionv[i];
    }

    /* update e_shnum */
    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(Elf64_Ehdr));
    ehdr.e_shnum = sectionv.size();

    if (!p_shstrtab) {
        if (!p_strtab)
            err(EXIT_FAILURE, "no .strtab nor .shstrtab");
        p_shstrtab = p_strtab;
    }

    /* allocate the memory for .shstrtab */
    int shname_sz = 0;
    for (int i = 0; i < sectionv.size(); ++i) {
        shname_sz += sectionv[i].name.size()+1;
    }
    p_shstrtab->capacity = p_shstrtab->size + shname_sz;
    unsigned char* tmp = new unsigned char[p_shstrtab->capacity];
    if (p_shstrtab->size && p_shstrtab->data) {
        memcpy(tmp, p_shstrtab->data, p_shstrtab->size);
        delete p_shstrtab->data;
    }
    p_shstrtab->data = tmp;

    /* update e_shstrndx and shdr's sh_name */
    for (int i = 0; i < sectionv.size(); ++i) {
        /* update shdr's sh_name */
        sectionv[i].shdr.sh_name = p_shstrtab->size;
        push_data(p_shstrtab, sectionv[i].name.c_str(), sectionv[i].name.size()+1);
    }
    ehdr.e_shstrndx = ((unsigned long)p_shstrtab
            - (unsigned long)sectionv.data()) / sizeof(section_t);

    /*
     * update section's sh_offset, sh_addr, sh_size and e_shoff
     * Note: sh will be located right after .shstrtab with 8 byte alignment
     */
    const int SH_ALIGN = 8;
    int offset = sizeof(Elf64_Ehdr);
    for (int i = 0; i < sectionv.size(); ++i) {
        /* update section's sh_offset, sh_addr */
        sectionv[i].shdr.sh_offset = offset;
        sectionv[i].shdr.sh_addr = 0;
        if (sectionv[i].shdr.sh_type == SHT_NOBITS)
            sectionv[i].shdr.sh_size = sectionv[i].capacity;
        else
            sectionv[i].shdr.sh_size = sectionv[i].size;

        offset = align(offset, sectionv[i].shdr.sh_addralign);
        offset += sectionv[i].size;
        offset = align(offset, sectionv[i].shdr.sh_addralign);

        if (&sectionv[i] == p_shstrtab) {
            offset = align(offset, SH_ALIGN);
            ehdr.e_shoff = offset;
            offset += (ehdr.e_shnum * sizeof(Elf64_Shdr));
        }
    }

    /* update e_entry */
    if (!p_symtab)
        err(EXIT_FAILURE, "no .symtab");
    else {
        ehdr.e_entry = sectionv[main_sym->st_shndx].shdr.sh_offset
            + main_sym->st_value;
    }

    /* write the ELF file */
    int fd;
    if ((fd = open(program_name, O_CREAT | O_WRONLY | O_TRUNC,
                    S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
        err(EXIT_FAILURE, "open \"%s\" failed", program_name);

    /* 1. ehdr */
    write_ehdr(fd, ehdr);

    /* 2. sections */
    for (int i = 0; i < sectionv.size(); ++i) {
        /* do not write anything for .bss section */
        if (sectionv[i].shdr.sh_type == SHT_NOBITS)
            continue;

        pwrite(fd, (char*)sectionv[i].data, sectionv[i].size, sectionv[i].shdr.sh_offset);

        if (&sectionv[i] == p_shstrtab) {
            /* write section headers */
            lseek(fd, ehdr.e_shoff, SEEK_SET);
            for (int j = 0; j < sectionv.size(); ++j) {
                write(fd, (char*)&sectionv[j].shdr, sizeof(Elf64_Shdr));
            }
        }

        delete (sectionv[i].data);
    }

    close(fd);
}
//-------- generate a linked relocatable file --------<

//-------- drop unnecessary relocatable files -------->
/*
 * assumption: we need only FUNC and OBJECT types
 *
 * only GLOBAL bind is visible to other file
 * i.e., the target of linking
 * --> WEAK is effect only when GLOBAL does not exist
 *
 * GLOBAL bind + NOTYPE type --> mem obj in the other file
 */
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
//-------- drop unnecessary relocatable files --------<

#define DEBUG 0

int main(int argc, const char *argv[])
{
    if (argc >= 2)
        program_name = argv[1];

    for (int i = 0; i < 128; ++i) {
        nop[i] = 0x90;
    }

    std::vector<string> v;
    string name;
    while (cin >> name) {
        v.push_back(name);
    }
    std::set<string> relocatables;
    select_needed_relocatables(v, relocatables);
    v.clear();

#if DEBUG /* print relocatable files */
    for (std::set<string>::iterator it = relocatables.begin();
            it != relocatables.end(); ++it) {
        cout << *it << endl;
    }
#endif

    for (std::map<string, Elf64_Shdr>::iterator it = shdr.begin();
            it != shdr.end(); ++it) {
#if DEBUG /* print section names */
        cout << it->first << ": " << (it->second).sh_size << endl;
#endif

        (it->second).sh_size = 0;
        (it->second).sh_link = 0;
        (it->second).sh_info = 0;

        section_t s = {it->first, it->second, NULL, 0, 0};
        sectionv.push_back(s);
    }

    for (std::map<string, string>::iterator it = shlink.begin();
            it != shlink.end(); ++it) {
#if DEBUG /* print section's link */
        cout << it->first << " => " << it->second << endl;
#endif

        for (int j = 0; j < sectionv.size(); ++j) {
            if (!sectionv[j].name.compare(it->first)) {
                for (int k = 0; k < sectionv.size(); ++k) {
                    if (!sectionv[k].name.compare(it->second)) {
                        sectionv[j].shdr.sh_link = k;
                        break;
                    }
                }
                break;
            }
        }
    }

    for (std::map<string, string>::iterator it = shinfo.begin();
            it != shinfo.end(); ++it) {
#if DEBUG /* print section's info */
        cout << it->first << " => " << it->second << endl;
#endif

        for (int j = 0; j < sectionv.size(); ++j) {
            if (!sectionv[j].name.compare(it->first)) {
                for (int k = 0; k < sectionv.size(); ++k) {
                    if (!sectionv[k].name.compare(it->second)) {
                        sectionv[j].shdr.sh_info = k;
                        break;
                    }
                }
                break;
            }
        }
    }

    for (std::set<string>::iterator it = relocatables.begin();
            it != relocatables.end(); ++it) {
        update_capacity(it->c_str());
    }

    /* add the default first symbol */
    section_t *symtab = get_section(".symtab\0");
    if (!symtab)
        err(EXIT_FAILURE, "no .symtab");
    Elf64_Sym sym_zero;
    memset(&sym_zero, 0, sizeof(Elf64_Sym));
    sym_zero.st_shndx = SHN_UNDEF;
    push_data(symtab, &sym_zero, sizeof(Elf64_Sym));

    for (std::set<string>::iterator it = relocatables.begin();
            it != relocatables.end(); ++it) {
        read_relocatable(it->c_str());
    }

    update_undef_rel();
    sort_sym();
    gen_relocatable();

#if DEBUG /* sizeof types --> to check and avoid type casting error */
    cout << "Elf64_Word: " << sizeof(Elf64_Word) << endl;
    cout << "Elf64_Xword: " << sizeof(Elf64_Xword) << endl;
    cout << "Elf64_Addr: " << sizeof(Elf64_Addr) << endl;
    cout << "Elf64_Off: " << sizeof(Elf64_Off) << endl;
#endif
    return 0;
}
