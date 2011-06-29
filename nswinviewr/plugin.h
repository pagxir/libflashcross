#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#define LOGPATH    "C:\\log.txt"

void NPN_ReleaseObject(NPObject *obj);
NPObject *NPN_RetainObject(NPObject *npobj);
NPObject *NPN_CreateObject(NPP npp, NPClass *aClass);

extern char __top_id[];
extern char __location_id[];
extern char __toString_id[];
extern char ___DoFSCommand_id[];

#endif

