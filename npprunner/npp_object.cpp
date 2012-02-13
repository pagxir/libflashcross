#include "npnapi.h"
#include "plugin.h"
#include "npp_object.h"

#ifndef ENAVLE_DEBUG
#define dbg_trace(fmt, args...) (void *)0
#else
#define dbg_trace(fmt, args...) fprintf(stderr, fmt, ##args)
#endif

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
	dbg_trace("ScriptObject::Invoke: %p %s %d\n", this, (char *)name, argCount);
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
	dbg_trace("ScriptObject::GetProperty:%p %s\n", this, (char *)name);
	return false;
}

bool ScriptObject::SetProperty(NPIdentifier name, const NPVariant *value)
{
	dbg_trace("ScriptObject::SetProperty: %s\n", (char *)name);
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
