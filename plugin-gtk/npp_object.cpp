#include "plugin.h"
#include "npp_object.h"

void ScriptObject::Invalidate()
{
}

bool ScriptObject::HasMethod(NPIdentifier name)
{
    return false;
}

bool ScriptObject::Invoke(NPIdentifier name, const NPVariant *args,
		 uint32_t argCount, NPVariant *result)
{
    return false;
}

bool ScriptObject::InvokeDefault(const NPVariant *args, uint32_t argCount,
	NPVariant *result)
{
    return false;
}

bool ScriptObject::HasProperty(NPIdentifier name)
{
    return false;
}

bool ScriptObject::GetProperty(NPIdentifier name, NPVariant *result)
{
    return false;
}

bool ScriptObject::SetProperty(NPIdentifier name, const NPVariant *value)
{
    return false;
}

bool ScriptObject::RemoveProperty(NPIdentifier name)
{
    return false;
}

bool ScriptObject::Enumerate(NPIdentifier **identifier, uint32_t *count)
{
    return false;
}

bool ScriptObject::Construct(const NPVariant *args, uint32_t argCount,
	NPVariant *result)
{
    return false;
}

ScriptObject::~ScriptObject()
{
}
