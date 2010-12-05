#include "npupp.h"
#include "npapi.h"

#include "plugin.h"
#include "npp_class.h"
#include "npp_object.h"

void ScriptClass::_Deallocate(NPObject *npobj)
{
    delete (ScriptObject*)npobj;
}

void ScriptClass::_Invalidate(NPObject *npobj)
{
    ((ScriptObject*)npobj)->Invalidate();
}

bool ScriptClass::_HasMethod(NPObject *npobj, NPIdentifier name)
{
    return ((ScriptObject*)npobj)->HasMethod(name);
}

bool ScriptClass::_Invoke(NPObject *npobj, NPIdentifier name,
	const NPVariant *args, uint32_t argCount,
	NPVariant *result)
{
    return ((ScriptObject*)npobj)->Invoke(name, args, argCount, result);
}

bool ScriptClass::_InvokeDefault(NPObject *npobj,
	const NPVariant *args,
	uint32_t argCount,
	NPVariant *result)
{
    return ((ScriptObject*)npobj)->InvokeDefault(args, argCount, result);
}

bool ScriptClass::_HasProperty(NPObject *npobj, NPIdentifier name)
{
    return  ((ScriptObject*)npobj)->HasProperty(name);
}

bool ScriptClass::_GetProperty(NPObject *npobj, NPIdentifier name,
	NPVariant *result)
{
    return  ((ScriptObject*)npobj)->GetProperty(name, result);
}

bool ScriptClass::_SetProperty(NPObject *npobj, NPIdentifier name,
	const NPVariant *value)
{
    return  ((ScriptObject*)npobj)->SetProperty(name, value);
}


bool ScriptClass::_RemoveProperty(NPObject *npobj, NPIdentifier name)
{
      return ((ScriptObject *)npobj)->RemoveProperty(name);
}


bool ScriptClass::_Enumerate(NPObject *npobj,
	NPIdentifier **identifier,
	uint32_t *count)
{
      return ((ScriptObject *)npobj)->Enumerate(identifier, count);
}


bool ScriptClass::_Construct(NPObject *npobj, const NPVariant *args,
	uint32_t argCount, NPVariant *result)
{
      return ((ScriptObject *)npobj)->Construct(args, argCount,
	      result);
}
