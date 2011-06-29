#include <assert.h>
#include <string.h>

#include "npnapi.h"
#include "plugin.h"
#include "npp_class.h"
#include "npp_object.h"
#include "npp_script.h"

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

		virtual bool HasProperty(NPIdentifier name);
		virtual bool GetProperty(NPIdentifier name, NPVariant *result);

	private:
		NPP m_npp;
};

	static NPObject *
allocScriptObject(NPP npp, NPClass *aClass)
{
	return new ScriptableControl(npp);
}

DECLARE_NPOBJECT_CLASS_WITH_BASE(ScriptObject, allocScriptObject);

NPObject *script_CreateObject(NPP instance)
{
	return NPN_CreateObject(instance,
			GET_NPOBJECT_CLASS(ScriptObject));
}

	ScriptableControl::ScriptableControl(NPP npp)
:m_npp(npp)
{
}

bool ScriptableControl:: HasProperty(NPIdentifier name)
{
	return false;
}

bool ScriptableControl::GetProperty(NPIdentifier name, NPVariant *result)
{
	return false;
}

class LocationObject: public ScriptObject
{
	public:
		virtual bool Invoke(NPIdentifier name, const NPVariant *args, uint32_t argCount, NPVariant *result);
};

bool LocationObject::Invoke(NPIdentifier name, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	printf("LocationObject::Invoke: %s\n", (char *)name);
	if (name == __toString_id){
		STRINGZ_TO_NPVARIANT(strdup("http://www.tudou.com/programs/view/nbeSbhYMOrg/"), *result);
		return true;
	}
	return false;
}


NPObject *allocLocationObject(NPP npp, NPClass *aClass)
{
	return new LocationObject();
}

DECLARE_NPOBJECT_CLASS_WITH_BASE(LocationObject, allocLocationObject);

class WindowObject: public ScriptObject
{
	public:
		WindowObject();
		~WindowObject();
		virtual bool GetProperty(NPIdentifier name, NPVariant *result);

	private:
		NPObject *mLocation;
};

NPObject *allocWindowObject(NPP npp, NPClass *aClass)
{
	return new WindowObject();
}

WindowObject::WindowObject()
{
	mLocation = NPN_CreateObject(NULL, GET_NPOBJECT_CLASS(LocationObject));
	assert(mLocation != NULL);
}

WindowObject::~WindowObject()
{
	NPN_ReleaseObject(mLocation);
}

bool WindowObject::GetProperty(NPIdentifier name, NPVariant *result)
{
	printf("WindowObject::GetProperty: %s %p\n", (char *)name, mLocation);
	if (name == __location_id){
		NPN_RetainObject(mLocation);
		OBJECT_TO_NPVARIANT(mLocation, *result);
		return true;
	}

	if (name == __top_id){
		NPN_RetainObject(this);
		OBJECT_TO_NPVARIANT(this, *result);
		return true;
	}

	return false;
}

NPClass *getWindowClass()
{
	DECLARE_NPOBJECT_CLASS_WITH_BASE(WindowObject, allocWindowObject);
	return GET_NPOBJECT_CLASS(WindowObject);
}

