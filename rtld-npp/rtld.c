#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/elf.h>

#include <elf.h>

#define VERBOSE 1

#include "rtld.h"
#include "elf_flags.h"

#define false 0
#define true 1
#define bool int

#define Elf_Addr Elf64_Addr
#define Elf_Sym  Elf64_Sym
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Dyn  Elf64_Dyn
#define Elf_Phdr Elf64_Phdr
#define Elf_Off  Elf64_Off
#define Elf_Rel  Elf64_Rel
#define Elf_Rela Elf64_Rela
#define Elf_Size Elf64_Off
#define Elf_Hashelt Elf32_Off
#define RTLD_DEFAULT 0
#define MAP_NOCORE 0

#define ELF_R_SYM ELF64_R_SYM
#define ELF_R_TYPE ELF64_R_TYPE
#define ELF_ST_TYPE ELF64_ST_TYPE
#define ELF_ST_BIND ELF64_ST_BIND
#define dlfunc dlsym

#ifndef ENABLE_DEBUG
#define dbg_trace(fmt, args...) (void *)0
#else
#define dbg_trace(fmt, args...) printf(fmt, ##args)
#endif

#define PAGESIZE (4096)
#define PAGEMASK (PAGESIZE - 1)
#define PROT_ALL (PROT_READ | PROT_WRITE | PROT_EXEC)
#define TRUNC_PAGE(vaddr) ((vaddr) & ~PAGEMASK)
#define ROUND_PAGE(vaddr) (((vaddr) + PAGEMASK) & ~PAGEMASK)

typedef unsigned char u_char;
__thread int _fixup_tls_var[2];

void fixup_init(void);
void _rtld_fixup_start(void);
void * fixup_lookup(const char * name, int in_plt);

static const Elf_Sym * find_symdef(
		unsigned long symnum, const Obj_Entry * refobj,
		const Obj_Entry ** defobj_out, bool in_plt, void * cache);

struct elf_object {
	unsigned long flags;
	Elf_Addr * pltgot;
	const Elf_Rel * rel;
	unsigned long relsize;
	const Elf_Rela * rela;
	unsigned long relasize;
	const Elf_Rel * pltrel;
	unsigned long pltrelsize;
	const Elf_Rela * pltrela;
	unsigned long pltrelasize;
	const Elf_Sym * symtab;
	const char * strtab;
	unsigned long strsize;
	const Elf_Hashelt * buckets;
	unsigned long nbuckets;
	const Elf_Hashelt * chains;
	unsigned long nchains;
	Elf_Addr init;
	Elf_Addr fini;

	const Elf_Dyn * dynamic;
	unsigned char * relocbase;
	unsigned long relocsize;
	unsigned long dl_count;
	void * * dl_handles;
};

static __thread Elf_Sym sym_zero;
static __thread Elf_Sym sym_temp;
static __thread Obj_Entry obj_main_0;

static int
convert_prot(int elfflags)
{
	int prot = 0;
	if (elfflags & PF_R)
		prot |= PROT_READ;

	if (elfflags & PF_W)
		prot |= PROT_WRITE;

	if (elfflags & PF_X)
		prot |= PROT_EXEC;
	return prot;
}

Elf_Addr _rtld_fixup(Obj_Entry * obj, Elf_Size reloff)
{
	const Elf_Sym * def;
	Elf_Addr * where;
	const Elf_Rel * rel;
	const Obj_Entry * defobj;

	rel = (Elf_Rel *)((char *)obj->pltrela + reloff);
	if (obj->pltrel != NULL)
		rel = (Elf_Rel *)((char *)obj->pltrel + reloff);
	where = (Elf_Addr *)(obj->relocbase + rel->r_offset);

	def = find_symdef(ELF_R_SYM(rel->r_info), obj, &defobj, true, NULL);
	if (def == NULL) {
		def = obj->symtab + ELF_R_SYM(rel->r_info);
		dbg_trace("symbol missing: %s\n", obj->strtab + def->st_name);
		exit(0);
	}

	const Elf_Sym * symp = obj->symtab + ELF_R_SYM(rel->r_info);
	const char * name = obj->strtab + symp->st_name;
	//dbg_trace("_rtld_bind ok: %s!\n", obj->strtab + symp->st_name);

	*where = (Elf_Addr)(defobj->relocbase + def->st_value);

	return (Elf_Addr)(defobj->relocbase + def->st_value);
}

int
elf_dlclose(const Obj_Entry * obj)
{
	unsigned long idx;
	void (* dl_fini)(void);

	if (obj != NULL && obj->fini) {
		dl_fini = (void (*)())obj->fini;
		dl_fini();
	}

	if (obj->dl_count > 0 && obj->dl_handles) {
		for (idx = obj->dl_count; idx > 0; idx--)
			if (obj->dl_handles[idx - 1] != NULL)
				dlclose(obj->dl_handles[idx - 1]);
		free(obj->dl_handles);
	}

	munmap(obj->relocbase, obj->relocsize);
	free((void *)obj);
	dbg_trace("elf_free is call!\n");
}

static void *
dlopen_wrap(const char * name, int mode)
{
	void * handle = 0;
	char buf[1024], * p;

	void * retval = dlopen(name, mode);
	if (retval != NULL)
		return retval;

	strncpy(buf, name, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;
	p = strstr(buf, ".so");
	if (p != NULL)
		*(p + 3) = 0;

	dbg_trace("dlopen_fixup %s %x\n", buf, mode);
	return dlopen(buf, mode);
}

static const Elf_Sym *
find_symdef(unsigned long symnum, const Obj_Entry * refobj,
		const Obj_Entry ** defobj_out, bool in_plt, void * cache)
{
	unsigned long idx;
	const void * symval;
	const char * name;
	const Elf_Sym * ref, * def;
	const Obj_Entry * defobj;

	if (symnum >= refobj->nchains) {
		dbg_trace("chain too large!\n");
		return NULL;
	}

	ref = refobj->symtab + symnum;
	name = refobj->strtab + ref->st_name;
	defobj = NULL;

	//dbg_trace("find_symdef: %s\n", name);
	assert(ELF_ST_TYPE(ref->st_info) != STT_SECTION);

	if (ELF_ST_BIND(ref->st_info) == STB_LOCAL) {
		def = ref;
		defobj = refobj;
		assert(ref->st_value != 0);
	} else if ((symval = fixup_lookup(name, in_plt)) != NULL) {
		def = &sym_temp;
		defobj = &obj_main_0;
		sym_temp.st_value = (Elf_Addr)symval;
	} else if ((symval = elf_dlsym(refobj, name)) != NULL){
		def = &sym_temp;
		defobj = &obj_main_0;
		sym_temp.st_value = (Elf_Addr)symval;
	} else {
		def = NULL;
		/* dbg_trace("symbol %s not found\n", name); */
	}

	while (def == NULL) {
		symval = in_plt? dlfunc(RTLD_DEFAULT, name): dlsym(RTLD_DEFAULT, name);
		if (symval == NULL)
			break;
		def = &sym_temp;
		defobj = &obj_main_0;
		sym_temp.st_value = (Elf_Addr)symval;
		break;
	}

	if (def == NULL && ELF_ST_BIND(ref->st_info) == STB_WEAK) {
		dbg_trace("unref weak object: %s\n", name);
		def = &sym_zero;
		defobj = &obj_main_0;
	}

	const char * fmt = in_plt? "jmpslot missing: %d %d %s\n":
		"rocslot missing: %d %d %s\n";
	if (def != NULL)
		*defobj_out = defobj;
	else if (in_plt == false)
		dbg_trace(fmt, ELF_ST_BIND(ref->st_info), ELF_ST_TYPE(ref->st_info), name);

	if (def == NULL && in_plt == false)
		fprintf(stderr, "symbol: %d %s\n", in_plt, name);
	assert (def != NULL || in_plt == true);
	return	def;
}

static int
digest_dynamic(Obj_Entry * obj)
{
	int plttype = DT_REL;
	const Elf_Dyn * dynp = NULL;
	unsigned long dl_count = 0;
	const Elf_Hashelt * hashtab;

	for (dynp = obj->dynamic; dynp->d_tag != DT_NULL; dynp++) {
		switch(dynp->d_tag) {
			case DT_NEEDED:
				dl_count++;
				break;

			case DT_SONAME:
				//dbg_trace("DT_NEEDED: %d\n", dynp->d_un.d_ptr);
				break;

			case DT_PLTRELSZ:
				assert((obj->flags & TF_PLTRELSZ) != TF_PLTRELSZ);
				obj->pltrelsize = dynp->d_un.d_val;
				obj->flags |= TF_PLTRELSZ;
				break;

			case DT_PLTGOT:
				assert((obj->flags & TF_PLTGOT) != TF_PLTGOT);
				obj->pltgot = (Elf_Addr *)(obj->relocbase + dynp->d_un.d_ptr);
				obj->flags |= TF_PLTGOT;
				break;

			case DT_HASH:
				assert((obj->flags & TF_HASH) != TF_HASH);
				hashtab = (const Elf_Hashelt *)
					(obj->relocbase + dynp->d_un.d_ptr);
				obj->nbuckets = hashtab[0];
				obj->nchains = hashtab[1];
				obj->buckets = (hashtab + 2);
				obj->chains = obj->buckets + obj->nbuckets;
				obj->flags |= TF_HASH;
				break;

			case DT_STRTAB:
				assert((obj->flags & TF_STRTAB) != TF_STRTAB);
				obj->strtab = (const char *)(obj->relocbase + dynp->d_un.d_ptr);
				break;

			case DT_SYMTAB:
				assert((obj->flags & TF_SYMTAB) != TF_SYMTAB);
				obj->symtab = (const Elf_Sym *)
					(obj->relocbase + dynp->d_un.d_ptr);
				obj->flags |= TF_SYMTAB;
				break;

			case DT_STRSZ:
				assert((obj->flags & TF_STRSZ) != TF_STRSZ);
				obj->strsize = dynp->d_un.d_val;
				obj->flags |= TF_STRSZ;
				break;

			case DT_SYMENT:
				assert((obj->flags & TF_SYMENT) != TF_SYMENT);
				assert((dynp->d_un.d_val) == sizeof(Elf_Sym));
				obj->flags |= TF_SYMENT;
				break;

			case 0x6ffffffc: /*->(21)*/
			case 0x6ffffffd: /*->(21)*/
			case 0x6ffffffe: /*->(21)*/
			case 0x6fffffff: /*->(21)*/
			case 0x6ffffffa: /*->(21)*/
			case 0x6ffffff0: /*->(21)*/
				dbg_trace("offset: %d\n",
						((unsigned char *)&(dynp->d_tag)) - obj->relocbase);
				dbg_trace("value: %x\n", dynp->d_un.d_val);
				break;

			case DT_INIT:
				dbg_trace("DT_INIT: %p - %d\n", dynp->d_un.d_ptr, getpid());
				assert((obj->flags & TF_INIT) != TF_INIT);
				obj->init = (Elf_Addr)(obj->relocbase + dynp->d_un.d_ptr);
				obj->flags |= TF_INIT;
				break;

			case DT_FINI:
				dbg_trace("DT_FINI: %p\n", dynp->d_un.d_ptr);
				assert((obj->flags & TF_FINI) != TF_FINI);
				obj->fini = (Elf_Addr)(obj->relocbase + dynp->d_un.d_ptr);
				obj->flags |= TF_FINI;
				break;

			case DT_REL:
				assert((obj->flags & TF_REL) != TF_REL);
				obj->rel = (const Elf_Rel *)(obj->relocbase + dynp->d_un.d_ptr);
				obj->flags |= TF_REL;
				break;

			case DT_RELSZ:
				assert((obj->flags & TF_RELSZ) != TF_RELSZ);
				obj->relsize = dynp->d_un.d_val;
				obj->flags |= TF_RELSZ;
				break;

			case DT_RELA:
				assert((obj->flags & TF_RELA) != TF_RELA);
				obj->rela = (const Elf_Rela *)(obj->relocbase + dynp->d_un.d_ptr);
				obj->flags |= TF_RELA;
				break;

			case DT_RELASZ:
				assert((obj->flags & TF_RELASZ) != TF_RELASZ);
				obj->relasize = dynp->d_un.d_val;
				obj->flags |= TF_RELASZ;
				break;

			case DT_RELAENT:
				assert((obj->flags & TF_RELAENT) != TF_RELAENT);
				assert((dynp->d_un.d_val) == sizeof(Elf_Rela));
				obj->flags |= TF_RELAENT;
				break;

			case DT_PLTREL:
				assert((obj->flags & TF_PLTREL) != TF_PLTREL);
				plttype = dynp->d_un.d_val;
				obj->flags |= TF_PLTREL;
				break;

			case DT_JMPREL:
				assert((obj->flags & TF_JMPREL) != TF_JMPREL);
				obj->pltrel = (const Elf_Rel *)
					(obj->relocbase + dynp->d_un.d_val);
				obj->flags |= TF_JMPREL;
				break;

			default:
				dbg_trace("unkown: dt_type %x\n", dynp->d_tag);
				break;
		}
	}

	if (dl_count > 0) {
		obj->dl_count = dl_count;
		obj->dl_handles = (void ** )malloc(dl_count * sizeof(void *));
		memset(obj->dl_handles, 0, dl_count * sizeof(void*));
		dl_count = 0;
	}

	for (dynp = obj->dynamic; dynp->d_tag != DT_NULL; dynp++) {
		const char * strp = 0;
		switch (dynp->d_tag) {
			case DT_NEEDED:
				strp = (const char *)(obj->strtab + dynp->d_un.d_ptr);
				obj->dl_handles[dl_count++] = dlopen_wrap(strp, RTLD_LAZY);
				dbg_trace("\tneeded: %s\n", strp);
				break;

			case DT_SONAME:
				dbg_trace("\tsoname: %s\n", obj->strtab + dynp->d_un.d_ptr);
				break;
		}
	}

	if (plttype == DT_RELA) {
		obj->pltrela = (const Elf_Rela *)obj->pltrel;
		obj->pltrelasize = obj->pltrelsize;
		obj->pltrel = NULL;
		obj->pltrelsize = 0;
		dbg_trace("DT_RELA\n");
	}

	return 0;
}

static int
elf_dlreloc(Obj_Entry * obj)
{
	Elf_Addr * where;
	const Elf_Sym * def;
	const Obj_Entry * defobj;

	const Elf_Rela * rela, * relalim;

	relalim = obj->rela + (obj->relasize / sizeof(*rela));
	for (rela = obj->rela; rela < relalim; rela++) {
		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);

		switch (ELF_R_TYPE(rela->r_info)) {
			case R_X86_64_64:
				def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
						false, NULL);
				assert(def != NULL);
				*where = (Elf_Addr) (defobj->relocbase + def->st_value + rela->r_addend);

			case R_X86_64_GLOB_DAT:
				def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
						false, NULL);
				assert(def != NULL);
				*where = (Elf_Addr) (defobj->relocbase + def->st_value);
				break;

			case R_X86_64_RELATIVE:
				*where = (Elf_Addr)(obj->relocbase + rela->r_addend);
				//dbg_trace("relative %p\n", where);
				break;

			default:
				dbg_trace("relslotdrop: type %x, bind %x\n",
						ELF_R_TYPE(rela->r_info), ELF_ST_BIND(rela->r_info));
				break;
		}
	}
	dbg_trace("NONPLTGOT relocate finish!\n");

	const Elf_Rela * pltrela, * pltrelalim;

	pltrelalim = obj->pltrela + (obj->pltrelasize / sizeof(*pltrela));
	for (pltrela = obj->pltrela; pltrela < pltrelalim; pltrela++) {
		where = (Elf_Addr *) (obj->relocbase + pltrela->r_offset);
		*where += (Elf_Addr)(obj->relocbase);
	}

#if 1
	for (pltrela = obj->pltrela; pltrela < pltrelalim; pltrela++) {
		where = (Elf_Addr *) (obj->relocbase + pltrela->r_offset);
		def = find_symdef(ELF_R_SYM(pltrela->r_info), obj, &defobj, true, NULL);
		if (def != NULL)
			*where = (Elf_Addr)(defobj->relocbase + def->st_value + pltrela->r_addend);
	}
#endif

	dbg_trace("PLTGOT relocate finish!\n");

	const Elf_Rel * rel, * rellim;

	rellim = obj->rel + (obj->relsize / sizeof(*rel));
	for (rel = obj->rel; rel < rellim; rel++) {
		where = (Elf_Addr *)(obj->relocbase + rel->r_offset);

		switch (ELF_R_TYPE(rel->r_info)) {
			case R_386_NONE:
				break;

			case R_386_GLOB_DAT:
				def = find_symdef(ELF_R_SYM(rel->r_info), obj, &defobj, false, NULL);
				assert(def != NULL);
				*where = (Elf_Addr)(defobj->relocbase + def->st_value);
				break;

			case R_386_RELATIVE:
				*where += (Elf_Addr)obj->relocbase;
				break;

			case R_386_32:
				def = find_symdef(ELF_R_SYM(rel->r_info), obj, &defobj, false, NULL);
				assert(def != NULL);
				*where += (Elf_Addr)(defobj->relocbase + def->st_value);
				break;

			case R_386_TLS_DTPMOD32:
#if 0
				def = find_symdef(ELF_R_SYM(rel->r_info), obj, &defobj, false, NULL);
				assert(def != NULL);
				*where ++;
#endif
				break;

			default:
				dbg_trace("unkown type: %d\n", ELF_R_TYPE(rel->r_info));
				assert(0);
				break;
		}
	}

	const Elf_Rel * pltrel, * pltrellim;

	pltrellim = obj->pltrel + (obj->pltrelsize / sizeof(*pltrel));
	for (pltrel = obj->pltrel; pltrel < pltrellim; pltrel++) {
		where = (Elf_Addr *) (obj->relocbase + pltrel->r_offset);
		*where += (Elf_Addr)(obj->relocbase);
	}

	for (pltrel = obj->pltrel; pltrel < pltrellim; pltrel++) {
		where = (Elf_Addr *) (obj->relocbase + pltrel->r_offset);
		def = find_symdef(ELF_R_SYM(pltrel->r_info), obj, &defobj, true, NULL);
		if (def != NULL)
			*where = (Elf_Addr)(defobj->relocbase + def->st_value);
	}

	if (obj->flags & TF_PLTGOT) {
		obj->pltgot[2] = (Elf_Addr)_rtld_fixup_start;
		obj->pltgot[1] = (Elf_Addr)obj;
	}

	return 0;
}

static int
elf_dlmmap(Obj_Entry * obj, int fd, Elf_Ehdr * hdr)
{
	int i = 0;
	int ptload1st = 0;
	const Elf_Phdr * phdr, * phdyn;
	const Elf_Phdr * phdrinit, * phdrfini;

	Elf_Off base_off;
	Elf_Addr base_vaddr, base_vlimit;

	Elf_Addr mapbase;
	size_t mapsize;

#ifdef VERBOSE
	dbg_trace("phoff: %ld\n", hdr->e_phoff);
	dbg_trace("ehsize: %ld\n", hdr->e_ehsize);
	dbg_trace("phnum: %ld\n", hdr->e_phnum);
	dbg_trace("phentsize: %ld\n", hdr->e_phentsize);
#endif

	phdrinit = (const Elf_Phdr *)((char *)hdr + hdr->e_phoff);
	phdrfini = (const Elf_Phdr *)(phdrinit + hdr->e_phnum);

	for (phdr = phdrinit; phdr < phdrfini; phdr++) {
#ifdef VERBOSE
		dbg_trace("\n--------Program Header[%d]---------\n", i++);
		dbg_trace("type: %x\n", phdr->p_type);
		dbg_trace("flags: %x\n", phdr->p_flags);
		dbg_trace("offset: %x\n", phdr->p_offset);
		dbg_trace("vaddr: %x\n", phdr->p_vaddr);
		dbg_trace("filesz: %x\n", phdr->p_filesz);
		dbg_trace("memsz: %x\n", phdr->p_memsz);
		dbg_trace("align: %x\n", phdr->p_align);
#endif
		switch (phdr->p_type) {
			case PT_INTERP:
				dbg_trace("INTERP\n");
				break;

			case PT_LOAD:
				if (ptload1st++ == 0)
					base_vaddr = TRUNC_PAGE(phdr->p_vaddr);
				base_vlimit = ROUND_PAGE(phdr->p_vaddr + phdr->p_memsz);
				break;

			case PT_PHDR:
				dbg_trace("PHDR\n");
				break;

			case PT_DYNAMIC:
				phdyn = phdr;
				break;

			case PT_TLS:
				dbg_trace("TLS\n");
				break;

			default:
				dbg_trace("Program Header Type: %x\n", phdr->p_type);
				break;
		}
	}

	mapsize = base_vlimit - base_vaddr;
	mapbase = (Elf_Addr)mmap(0, mapsize, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (mapbase == (Elf_Addr)MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	dbg_trace("mapsize %ld, %p\n", mapsize, mapbase);

	for (phdr = phdrinit; phdr < phdrfini; phdr++) {
		Elf_Off data_off;
		Elf_Addr data_addr;
		Elf_Addr data_vaddr;
		Elf_Addr data_vlimit;
		int data_prot, data_flags;

		if (phdr->p_type != PT_LOAD)
			continue;

		data_off = TRUNC_PAGE(phdr->p_offset);
		data_vaddr = TRUNC_PAGE(phdr->p_vaddr);
		data_vlimit = ROUND_PAGE(phdr->p_vaddr + phdr->p_filesz);

		data_addr = mapbase + (data_vaddr - base_vaddr);
		data_prot = convert_prot(phdr->p_flags);
		data_flags = MAP_FIXED | MAP_PRIVATE | MAP_NOCORE;

		if (mmap((void *)data_addr, data_vlimit - data_vaddr, data_prot| PROT_WRITE,
					data_flags, fd, data_off) == MAP_FAILED) {
			perror("mmap");
			return -1;
		}

		if (phdr->p_filesz != phdr->p_memsz) {
			ssize_t nclear;
			void * clear_page;
			Elf_Addr clear_vaddr, clear_addr;
			clear_vaddr = phdr->p_vaddr + phdr->p_filesz;
			clear_addr  = mapbase + (clear_vaddr - base_vaddr);
			clear_page  = (void *)(mapbase + (TRUNC_PAGE(clear_vaddr) - base_vaddr));

			if ((nclear = data_vlimit - clear_vaddr) > 0) {
				Elf_Addr bss_vaddr, bss_addr, bss_vlimit;

				if ((data_prot & PROT_WRITE) == 0)
					mprotect(clear_page, PAGESIZE, data_prot|PROT_WRITE);

				memset((void *)clear_addr, 0, nclear);

				if ((data_prot & PROT_WRITE) == 0)
					mprotect(clear_page, PAGESIZE, data_prot);

				bss_vaddr = data_vlimit;
				bss_vlimit = ROUND_PAGE(phdr->p_vaddr + phdr->p_memsz);
				bss_addr = mapbase + (bss_vaddr - base_vaddr);

				if (mprotect((void *)bss_addr, bss_vlimit - bss_vaddr, data_prot) == -1) {
					perror("mprotect");
					return -1;
				}
			}
		}
	}

	obj->flags = 0;
	obj->relocsize = mapsize;
	obj->relocbase = (unsigned char *)mapbase;
	obj->dl_count = 0;
	obj->dl_handles = NULL;
	obj->dynamic = (const Elf_Dyn *)(phdyn->p_vaddr + obj->relocbase);

	return 0;
}

unsigned long
elf_hash(const char * name)
{
	const unsigned char * p = (const unsigned char *) name;
	unsigned long h = 0;
	unsigned long g;

	while (*p != '\0') {
		h = (h << 4) + *p++;
		if ((g = h & 0xF0000000) != 0)
			h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

const void *
elf_dlsym(const Obj_Entry * obj, const char * name)
{
	unsigned long symnum;
	assert(obj != NULL);

	if ((obj->flags & TF_HASH) != TF_HASH)
		return 0;

	unsigned long hash = elf_hash(name);
	symnum = obj->buckets[hash % obj->nbuckets];
	for (; symnum != STN_UNDEF; symnum = obj->chains[symnum]) {
		const char * strp;
		const Elf_Sym * symp;

		if (symnum >= obj->nchains)
			return NULL;

		symp = obj->symtab + symnum;
		strp = obj->strtab + symp->st_name;

		switch (ELF_ST_TYPE(symp->st_info)) {
			case STT_FUNC:
			case STT_NOTYPE:
			case STT_OBJECT:
				if (symp->st_value == 0)
					continue;
			case STT_TLS:
				if (symp->st_shndx != SHN_UNDEF)
					break;
			default:
				continue;
		}

		if (*name != *strp || strcmp(name, strp) != 0)
			continue;

		return (obj->relocbase + symp->st_value);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	int i;
	for (i=1; i<argc; i++){
		const char * (*getMIMEDescription)(void);
		const Obj_Entry * obj = elf_dlopen(argv[i]);
		assert(obj != NULL);
		getMIMEDescription = (const char * (*)())elf_dlsym(obj, "NP_GetMIMEDescription");
		if (getMIMEDescription != NULL)
			dbg_trace("aux_add return %s\n", getMIMEDescription());
		elf_dlclose(obj);
	}
	return 0;
}

const Obj_Entry *
elf_dlopen(const char *path)
{
	int fd;
	struct stat sb;
	ssize_t nbytes;
	Obj_Entry * obj;
	void (* dl_init)(void);

	union {
		Elf_Ehdr hdr;
		char buf[PAGESIZE];
	}u;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return NULL;

	if (fstat(fd, &sb) == -1) {
		close(fd);
		return NULL;
	}

	nbytes = pread(fd, u.buf, PAGESIZE, 0);
	if (nbytes == -1) {
		close(fd);
		return NULL;
	}

	if ((size_t)nbytes < sizeof(Elf_Ehdr)) {
		close(fd);
		return NULL;
	}

	if (u.hdr.e_phentsize != sizeof(Elf_Phdr)) {
		close(fd);
		return NULL;
	}

	if (u.hdr.e_phoff + u.hdr.e_phnum * sizeof(Elf_Phdr) > (size_t)nbytes) {
		close(fd);
		return NULL;
	}

	obj = (Obj_Entry *)malloc(sizeof(Obj_Entry));
	if (obj == NULL) {
		close(fd);
		return NULL;
	}

	memset(obj, 0, sizeof(*obj));
	if (0 != elf_dlmmap(obj, fd, &u.hdr)) {
		close(fd);
		free(obj);
		return NULL;
	}

	close(fd);
	fixup_init();

	if (0 != digest_dynamic(obj)) {
		dbg_trace("digest_dynamic fail\n");
		munmap(obj->relocbase, obj->relocsize);
		free(obj);
		return NULL;
	}

	if (0 != elf_dlreloc(obj)) {
		dbg_trace("elf_dlreloc fail\n");
		munmap(obj->relocbase, obj->relocsize);
		free(obj);
		return NULL;
	}
	dbg_trace("elf_dlreloc success\n");

	if (obj->init != 0) {
		dl_init = (void (*)(void))obj->init;
		dl_init();
	}

	dbg_trace("init call finish!\n");
	return obj;
}
