#ifndef NANAOS_ELF_H
#define NANAOS_ELF_H

#include <stdint.h>
#include <stddef.h>

/* Minimal ELF64 structures/consts for loader */
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t  Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;

#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off  e_phoff;
    Elf64_Off  e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} __attribute__((packed)) Elf64_Phdr;

/* e_ident offsets */
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define EI_CLASS 4
#define ELFCLASS64 2

/* e_type values */
#define ET_EXEC 2
#define ET_DYN  3

/* Machine */
#define EM_X86_64 62

/* Program header types */
#define PT_NULL 0
#define PT_LOAD 1

/* Validate basic ELF64 header and program headers */
int elf_validate(const void *data, size_t size, Elf64_Ehdr *out_hdr);

/* Read program header (ensure bounds) */
int elf_read_phdr(const void *data, size_t size, const Elf64_Ehdr *ehdr, unsigned idx, Elf64_Phdr *out_phdr);

#endif /* NANAOS_ELF_H */
