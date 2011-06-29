#define MG(T,f) f##T
#define FORWARD(F) __stdcall forward_##F

//define LOGPATH "C:\\Applications\\Firefox\\plugins\\log.txt"
#define LOGPATH "C:\\log.txt"

typedef int forward_call();

#define FORWARD_CALL(T,f) int f##T(){ \
	/*MessageBox(0, #f, #T, 0);*/ \
	return 0; \
}

void NPN_ReleaseObject(NPObject *obj);
NPObject *NPN_RetainObject(NPObject *npobj);
NPObject *NPN_CreateObject(NPP npp, NPClass *aClass);

extern char __top_id[];
extern char __location_id[];
extern char __toString_id[];
extern char ___DoFSCommand_id[];
