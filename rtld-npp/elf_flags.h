#ifndef _ELF_FLAG_H
#define _ELF_FLAG_H

#define TF_NULL        (1 <<  0)
#define TF_NEEDED      (1 <<  1)
#define TF_PLTRELSZ    (1 <<  2)
#define TF_PLTGOT      (1 <<  3)
#define TF_HASH        (1 <<  4)
#define TF_STRTAB      (1 <<  5)
#define TF_SYMTAB      (1 <<  6)
#define TF_RELA        (1 <<  7)
#define TF_RELASZ      (1 <<  8)
#define TF_RELAENT     (1 <<  9)
#define TF_STRSZ       (1 << 10)
#define TF_SYMENT      (1 << 11)
#define TF_INIT        (1 << 12)
#define TF_FINI        (1 << 13)
#define TF_SONAME      (1 << 14)
#define TF_RPATH       (1 << 15)
#define TF_SYMBOLIC    (1 << 16)
#define TF_REL	       (1 << 17)
#define TF_RELSZ       (1 << 18)
#define TF_RELENT      (1 << 19)
#define TF_PLTREL      (1 << 20)
#define TF_DEBUG       (1 << 21)
#define TF_TEXTREL     (1 << 22)
#define TF_JMPREL      (1 << 23)

#endif
