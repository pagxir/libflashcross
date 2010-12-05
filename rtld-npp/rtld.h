#ifndef _RTLD_H
#define _RTLD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct elf_object Obj_Entry;
const Obj_Entry * elf_dlopen(const char * path);
const void * elf_dlsym(const Obj_Entry *, const char * name);
int elf_dlclose(const Obj_Entry * obj);

#ifdef __cplusplus
}
#endif

#define NSAPI(name) name##_fixup
#endif
