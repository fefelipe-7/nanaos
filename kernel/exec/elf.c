#include "exec/elf.h"
#include "lib/string.h"
#include <stdint.h>
#include <stddef.h>

int elf_validate(const void *data, size_t size, Elf64_Ehdr *out_hdr) {
    if (!data || size < sizeof(Elf64_Ehdr) || !out_hdr) return -1;
    const unsigned char *b = (const unsigned char *)data;
    if (b[0] != ELFMAG0 || b[1] != ELFMAG1 || b[2] != ELFMAG2 || b[3] != ELFMAG3) return -2;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)data;
    if (eh->e_ident[EI_CLASS] != ELFCLASS64) return -3;
    if (eh->e_machine != EM_X86_64) return -4;
    if (eh->e_type != ET_EXEC && eh->e_type != ET_DYN) return -5;
    /* basic program header bounds check */
    if (eh->e_phoff == 0 || eh->e_phnum == 0) return -6;
    if (eh->e_phoff + (size_t)eh->e_phnum * eh->e_phentsize > size) return -7;
    /* copy header out */
    *out_hdr = *eh;
    return 0;
}

int elf_read_phdr(const void *data, size_t size, const Elf64_Ehdr *ehdr, unsigned idx, Elf64_Phdr *out_phdr) {
    if (!data || !ehdr || !out_phdr) return -1;
    if (idx >= ehdr->e_phnum) return -2;
    size_t off = ehdr->e_phoff + (size_t)idx * ehdr->e_phentsize;
    if (off + sizeof(Elf64_Phdr) > size) return -3;
    const Elf64_Phdr *ph = (const Elf64_Phdr *)((const unsigned char *)data + off);
    *out_phdr = *ph;
    return 0;
}
