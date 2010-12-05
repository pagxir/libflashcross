#include "plugin.h"
#include "npp_class.h"
#include "npp_object.h"

#define DECLARE_NPOBJECT_CLASS_WITH_BASE(_class, ctor)			\
    static NPClass s##_class##_NPClass = {				\
	  NP_CLASS_STRUCT_VERSION_CTOR, 				\
	  ctor, 							\
	  ScriptClass::_Deallocate,				       \
	  ScriptClass::_Invalidate,				       \
	  ScriptClass::_HasMethod,				       \
	  ScriptClass::_Invoke, 				       \
	  ScriptClass::_InvokeDefault,				       \
	  ScriptClass::_HasProperty,				       \
	  ScriptClass::_GetProperty,				       \
	  ScriptClass::_SetProperty,				       \
	  ScriptClass::_RemoveProperty, 			       \
	  ScriptClass::_Enumerate,				       \
	  ScriptClass::_Construct				       \
    }

#define GET_NPOBJECT_CLASS(_class) &s##_class##_NPClass

class ScriptableControl: public ScriptObject
{
    public:
	ScriptableControl(NPP npp);

	 virtual bool HasMethod(NPIdentifier name);
	 virtual bool Invoke(NPIdentifier name, const NPVariant *args,
		 uint32_t argCount, NPVariant *result);

	 virtual bool HasProperty(NPIdentifier name);
	 virtual bool GetProperty(NPIdentifier name, NPVariant *result);

    private:
	CPlugin *m_plugin;
	NPP m_npp;
};

static NPObject *
allocScriptObject(NPP npp, NPClass *aClass)
{
    return new ScriptableControl(npp);
}

DECLARE_NPOBJECT_CLASS_WITH_BASE(ScriptClass, allocScriptObject);

NPObject *script_CreateObject(NPP instance)
{
    return NPN_CreateObject(instance,
	    GET_NPOBJECT_CLASS(ScriptClass));
}

ScriptableControl::ScriptableControl(NPP npp)
:m_npp(npp)
{
    m_plugin = (CPlugin*)npp->pdata;
}

bool ScriptableControl:: HasMethod(NPIdentifier name)
{
    if (name == CPlugin::sEchoTest_id)
	return true;
    if (name == CPlugin::sCi1_id)
	return true;
    return false;
}

bool ScriptableControl::Invoke(NPIdentifier name, const NPVariant *args,
	uint32_t argCount, NPVariant *result)
{
    if (name == CPlugin::sEchoTest_id){
	INT32_TO_NPVARIANT(1, *result);
	return true;
    }
    if (name == CPlugin::sCi1_id){
	STRINGZ_TO_NPVARIANT(strdup(""), *result);
	return true;
    }
    return false;
}

bool ScriptableControl:: HasProperty(NPIdentifier name)
{
    if (name == CPlugin::sTextData_id)
	return true;
    return false;
}

bool ScriptableControl::GetProperty(NPIdentifier name, NPVariant *result)
{
    if (name == CPlugin::sTextData_id){
	STRINGZ_TO_NPVARIANT(m_plugin->dupText(), *result);
	return true;
    }
    return false;
}
