typedef unsigned long addr_t;

extern char __sgx_start;        /* defined in the linker script */
extern char __sgx_end;          /* defined in the linker script */
extern char __sgx_code;         /* defined in the linker script */
#define _SGX_SIZE 0x2000000
#define _SGXCODE_BASE ((void*)&__sgx_code)
#define _SGXDATA_BASE ((void*)((addr_t)&__sgx_code + __sgx_data_ofs))

unsigned char *program = (unsigned char *)&__sgx_start;
size_t program_size = (addr_t)&__sgx_end - (addr_t)&__sgx_start;

#include <endian.h>
#if BYTE_ORDER == BIG_ENDIAN
# define byteorder ELFDATA2MSB
#elif BYTE_ORDER == LITTLE_ENDIAN
# define byteorder ELFDATA2LSB
#else
# error "Unknown BYTE_ORDER " BYTE_ORDER
# define byteorder ELFDATANONE
#endif

#define GET_OBJ(type, offset) \
     reinterpret_cast<type*>( reinterpret_cast<size_t>(program) \
            + static_cast<size_t>(offset) )
#define CHECK_SIZE(obj, size) \
    ((addr_t)obj + size <= (addr_t)program + program_size)

static Elf64_Ehdr *pehdr;
static Elf64_Shdr *pshdr;
static size_t n_symtab, _n_symtab = 1;
static Elf64_Sym *symtab;
char *strtab;
static Elf64_Sym *main_sym;

/*
 * After loading the metadata,
 * rel.r_offset = [ the index of the relocation source : the offset from the source ]
 */
size_t n_rel;               /* # of relocation tables */
size_t *n_reltab;           /* # of relocation entry */
static Elf64_Rela **reltab; /* array of pointers to relocation tables */
#define REL_DST_NDX(ofs) ((ofs) >> 32)
#define REL_DST_OFS(ofs) ((ofs) & 0xffffffff)
#define REL_OFFSET(ndx, ofs) ((((unsigned long)(ndx)) << 32) | ((unsigned long)(ofs)))

uint64_t rounddown(uint64_t align, uint64_t n)
{
    return (n / align) * align;
}

static uint32_t get_rand(void)
{
    uint32_t val;
    sgx_read_rand((unsigned char *)&val, sizeof(uint32_t));
    return val;
}

#if RAND
void *reserve_data(size_t size, size_t align)
{
    uint64_t ofs;
    do {
        ofs = rounddown(align, get_rand() % (_SGX_SIZE - size)) + (uint64_t)_SGXDATA_BASE;
        for (unsigned i = 1;i < _n_symtab;++i)
            if ((symtab[i].st_value <= ofs && ofs < symtab[i].st_value + symtab[i].st_size)
                    || (symtab[i].st_value < ofs + size
                        && ofs + size <= symtab[i].st_value + symtab[i].st_size)
                    || (ofs <= symtab[i].st_value && symtab[i].st_value < ofs + size)
                    || (ofs < symtab[i].st_value + symtab[i].st_size
                        && symtab[i].st_value + symtab[i].st_size <= ofs + size)) {
                ofs = 0;
                break;
            }
    } while (!ofs || (ofs + size > (uint64_t)_SGXDATA_BASE + _SGX_SIZE));
    return (void *)ofs;
}
#else
void *reserve_data(size_t size, size_t align)
{
    static void *data_end = _SGXDATA_BASE;
    void *ret = (void *)rounddown(align, (addr_t)data_end+(align-1));
    data_end = (void *)((addr_t)ret+rounddown(align, size+(align-1)));
    return ret;
}
#endif

bool is_available(uint8_t *base, size_t index, size_t size)
{
    for (unsigned i = 0;i < size;++i)
        if (base[index+i]) return false;
    return true;
}

#if RAND
void *reserve_code(size_t size, size_t align)
{
    uint64_t index;
    do index = rounddown(align, get_rand() % (_SGX_SIZE - size));
    while (!is_available((uint8_t *)_SGXCODE_BASE, index, size));
    return (void *)((uint64_t)_SGXCODE_BASE + index);
}
#else
void *reserve_code(size_t size, size_t align)
{
    static void *code_end = _SGXCODE_BASE;
    void *ret = (void *)rounddown(align, (addr_t)code_end+(align-1));
    code_end = (void *)((addr_t)ret+rounddown(align, size+(align-1)));
    return ret;
}
#endif

void *reserve(Elf64_Xword flags, size_t size, size_t align)
{
    if (flags & 0x4) return reserve_code(size, align);
    return reserve_data(size, align);
}

#define STR_EQUAL(s1, s2, n) \
    str_equal((const uint8_t *)(s1), (const uint8_t *)(s2), (n))
uint8_t str_equal(const uint8_t *s1, const uint8_t *s2, size_t n) {
    for (unsigned i = 0;i < n;++i)
        if (s1[i] != s2[i])
            return 0;
    return 1;
}
static void validate_ehdr(void)
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

    if ((pehdr = GET_OBJ(Elf64_Ehdr, 0)) == NULL)
        dlog("%u: Ehdr size", __LINE__);

    if (!str_equal(pehdr->e_ident, expected, EI_ABIVERSION)
            || pehdr->e_ident[EI_ABIVERSION] != 0
            || !str_equal(&pehdr->e_ident[EI_PAD], &expected[EI_PAD],
                EI_NIDENT - EI_PAD))
        dlog("%u: Ehdr ident", __LINE__);

    if (pehdr->e_version != EV_CURRENT)
        dlog("%u: Ehdr version", __LINE__);

    /* ELF format check - relocatable */
    if (pehdr->e_type != ET_REL)
        dlog("%u: Ehdr not relocatable", __LINE__);

    /* check the architecture - currently only support x86_64 */
    if (pehdr->e_machine != EM_X86_64)
        dlog("%u: Ehdr not x86_64", __LINE__);

    if (pehdr->e_shentsize != sizeof (Elf64_Shdr))
        dlog("%u: Shdr entry size", __LINE__);
}

void *get_buf(size_t size) {
    static addr_t heap_end = _HEAP_BASE;
    void *ret = (void *)heap_end;
    heap_end = heap_end + size;
    return ret;
}

/* search (section SE, OFS) from symtab - binary search can be applied */
static unsigned search(const Elf64_Half se, const Elf64_Addr ofs)
{
    // assuming that symbols are already sorted
    for (unsigned i = 0; i < n_symtab; ++i)
      if (symtab[i].st_shndx == se && symtab[i].st_value <= ofs
          && (i+1 >= n_symtab || symtab[i+1].st_value > ofs
              || symtab[i+1].st_shndx != se)) return i;
    return -1;
}

static void update_reltab(void)
{
    /* read shdr */
    if ((pshdr = GET_OBJ(Elf64_Shdr, pehdr->e_shoff)) == NULL
            || !CHECK_SIZE(pshdr, pehdr->e_shnum*sizeof(Elf64_Shdr)))
        dlog("%u: Shdr size", __LINE__);

    /* pointers to symbol, string, relocation tables */
    n_rel = 0;
    for (unsigned i = 0; i < pehdr->e_shnum; ++i) {
        if (pshdr[i].sh_type == SHT_RELA) ++n_rel;
        else if (pshdr[i].sh_type == SHT_SYMTAB) {
            symtab = GET_OBJ(Elf64_Sym, pshdr[i].sh_offset);
            n_symtab = pshdr[i].sh_size / sizeof(Elf64_Sym);
        } else if (pshdr[i].sh_type == SHT_STRTAB)
            strtab = GET_OBJ(char, pshdr[i].sh_offset);
    }
    n_reltab = (size_t *)get_buf(n_rel * sizeof(size_t));
    reltab = (Elf64_Rela **)get_buf(n_rel * sizeof(Elf64_Rela *));
    n_rel = 0;
    for (unsigned i = 0; i < pehdr->e_shnum; ++i) {
        if (pshdr[i].sh_type == SHT_RELA && pshdr[i].sh_size) {
            reltab[n_rel] = GET_OBJ(Elf64_Rela, pshdr[i].sh_offset);
            n_reltab[n_rel] = pshdr[i].sh_size / sizeof(Elf64_Rela);

            /* update relocation table: r_offset --> dst + offset */
            // assert(GET_OBJ(pshdr[pshdr[i].sh_link].sh_offset) == symtab);
            for (size_t j = 0; j < n_reltab[n_rel]; ++j) {
                unsigned dst = search(pshdr[i].sh_info, reltab[n_rel][j].r_offset);
                reltab[n_rel][j].r_offset =
                    REL_OFFSET(dst, reltab[n_rel][j].r_offset - symtab[dst].st_value);
            }
            ++n_rel;
        }
    }
}

static void fill_zero(char *ptr, Elf64_Word size) {
    while (size--) ptr[size] = 0;
}
static void cpy(char *dst, char *src, size_t size) {
    while (size--) dst[size] = src[size];
}

#ifndef NOSGX
#include "ocall_stub.cpp"
#include "ocall_stub_table.cpp"
#else
#include "nosgx_ocall_stub.cpp"
#endif
static unsigned char find_special_symbol(const char* name, const size_t i)
{
    if (STR_EQUAL(name, "dep.bdr\0", 8)) {
        symtab[i].st_value = (Elf64_Addr)_SGXDATA_BASE;
        symtab[i].st_size = 0;
        dlog(&strtab[symtab[i].st_name]);
        return 1;
    } else if (STR_EQUAL(name, "ocall.bdr\0", 10)) {
        symtab[i].st_value = (Elf64_Addr)_SGXCODE_BASE;
        symtab[i].st_size = 0;
        dlog(&strtab[symtab[i].st_name]);
        return 1;
    } else if (STR_EQUAL(name, "sgx_ocall.loader\0", 14)) {
        symtab[i].st_value = (Elf64_Addr)do_sgx_ocall;
        dlog("%s: %lx", &strtab[symtab[i].st_name], symtab[i].st_value);
        return 1;
    } else if (STR_EQUAL(name, "rand_internal\0", 14)) {
        symtab[i].st_value = (Elf64_Addr)get_rand;
        dlog("%s: %lx", &strtab[symtab[i].st_name], symtab[i].st_value);
        return 1;
    } else if (STR_EQUAL(name, "_stack\0", 7)) {
        symtab[i].st_value = (Elf64_Addr)reserve_data(symtab[i].st_size, 64);
        dlog("%s: %lx", &strtab[symtab[i].st_name], symtab[i].st_value);
        return 1;
    }
    return 0;
}

static void load(void)
{
    Elf64_Addr last_off = (Elf64_Addr)-1;
    Elf64_Addr last_st_value = (Elf64_Addr)-1;
    Elf64_Xword last_size = 0;
    unsigned shndx = -1;
    for (unsigned i = 1; i < n_symtab; ++i, ++_n_symtab) {
        if (shndx != symtab[i].st_shndx) {
            last_off = (Elf64_Addr)-1;
            last_st_value = (Elf64_Addr)-1;
            last_size = 0;
            shndx = symtab[i].st_shndx;
        }
        unsigned char found = symtab[i].st_name ?
            find_special_symbol(&strtab[symtab[i].st_name], i) : 0;
        /* special shndx --> assumption: no abs, no undef */
        if (symtab[i].st_shndx == SHN_COMMON && !found) {
            symtab[i].st_value = (Elf64_Addr)reserve(0, symtab[i].st_size, symtab[i].st_value);
            fill_zero((char *)symtab[i].st_value, symtab[i].st_size);
        } else if (!found) {
            Elf64_Addr symoff = pshdr[symtab[i].st_shndx].sh_offset + symtab[i].st_value;
            /* potentially WEAK bind */
            if (last_off <= symoff && symoff < (last_off + last_size)) {
                symtab[i].st_value = last_st_value + symoff - last_off;
            } else {
                /* find main */
                if (symoff == pehdr->e_entry)
                    main_sym = &symtab[i];

                symtab[i].st_value = (Elf64_Addr)reserve(pshdr[symtab[i].st_shndx].sh_flags,
                        symtab[i].st_size, pshdr[symtab[i].st_shndx].sh_addralign);

                /* fill zeros for .bss section .. otherwise, copy from file */
                if (pshdr[symtab[i].st_shndx].sh_type == SHT_NOBITS) {
                    fill_zero((char *)symtab[i].st_value, symtab[i].st_size);
                } else {
                    cpy((char *)symtab[i].st_value, GET_OBJ(char, symoff), symtab[i].st_size);

                    /* update last values */
                    last_size = symtab[i].st_size;
                    last_off = symoff;
                    last_st_value = symtab[i].st_value;
                }
            }
        }
        dlog("sym %04u %08lx", i, (unsigned long)symtab[i].st_value);
#if PTRACE
        find_ptrace_target(&strtab[symtab[i].st_name], i);
#endif
    }
}

static void relocate(void)
{
    for (unsigned k = 0; k < n_rel; ++k)
        for (unsigned i = 0; i < n_reltab[k]; ++i) {
            unsigned int ofs = REL_DST_OFS(reltab[k][i].r_offset);
            unsigned int dst_sym = REL_DST_NDX(reltab[k][i].r_offset);
            unsigned int src_sym = ELF64_R_SYM(reltab[k][i].r_info);
            const unsigned int type = ELF64_R_TYPE(reltab[k][i].r_info);
            addr_t dst = (addr_t)symtab[dst_sym].st_value + (addr_t)ofs;
#if PTRACE
            check_breakpoint(src_sym, dst);
#endif

            dlog("rel[%04u] %04u (%08lx) --> %04u", i, dst_sym, dst, src_sym);
            if (type == R_X86_64_64) {
                /* word 64 */
                *(addr_t *)dst = symtab[src_sym].st_value + reltab[k][i].r_addend;
                dlog("%lx", *(addr_t *)dst);
            } else if (type == R_X86_64_32) {
                /* word 32 */
                *(uint32_t*)dst = (uint32_t)(symtab[src_sym].st_value + reltab[k][i].r_addend);
                dlog("%x", *(uint32_t *)dst);
            } else if (type == R_X86_64_32S) {
                /* word 32 */
                *(int32_t*)dst = (int32_t)(symtab[src_sym].st_value + reltab[k][i].r_addend);
                dlog("%x", *(int32_t *)dst);
            } else if (type == R_X86_64_PC32 || type == R_X86_64_PLT32) {
                /* word 32 */
                *(uint32_t*)dst = (uint32_t)(symtab[src_sym].st_value
                        - dst + reltab[k][i].r_addend);
                dlog("%x", *(uint32_t *)dst);
            } else if (type == R_X86_64_GOTPCREL) {
                /* word 32 */
                *(uint32_t*)dst = (uint32_t)((Elf64_Addr)&(symtab[src_sym].st_value)
                        - dst + reltab[k][i].r_addend);
                dlog("%x", *(uint32_t *)dst);
            } else
                dlog("%u: Relocation -- not supported type %u", __LINE__, type);
        }
}

void enclave_main()
{
    void (*entry)();
    dlog("program at %p (%lx)", program, program_size);
    dlog(".sgxcode = %p", _SGXCODE_BASE);
    dlog(".sgxdata = %p", _SGXDATA_BASE);
    sgx_push_gadget((unsigned long)_SGXCODE_BASE);
    sgx_push_gadget((unsigned long)_SGXDATA_BASE);

    dlog("__sgx_start = %p", &__sgx_start);
    dlog("__sgx_end = %p", &__sgx_end);
    dlog("__elf_end = %p", &__elf_end);
    dlog("heap base = %lx", _HEAP_BASE);

    validate_ehdr();
    update_reltab();
    load();
    relocate();

    entry = (void (*)())(main_sym->st_value);
    dlog("main: %p", entry);

#if PTRACE
    run_with_ptrace(entry);
#else
    __asm__ __volatile__( "push %%r13\n" "push %%r14\n" "push %%r15\n" ::);
    entry();
    __asm__ __volatile__( "pop %%r15\n" "pop %%r14\n" "pop %%r13\n" ::);
#endif
}
