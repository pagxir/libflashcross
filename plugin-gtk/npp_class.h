
class ScriptClass
{
	public:
		static void _Deallocate(NPObject *npobj);
		static void _Invalidate(NPObject *npobj);
		static bool _HasMethod(NPObject *npobj, NPIdentifier name);
		static bool _Invoke(NPObject *npobj, NPIdentifier name,
				const NPVariant *args, uint32_t argCount, NPVariant *result);
		static bool _InvokeDefault(NPObject *npobj, const NPVariant *args,
				uint32_t argCount, NPVariant *result);
		static bool _HasProperty(NPObject * npobj, NPIdentifier name);
		static bool _GetProperty(NPObject *npobj, NPIdentifier name,
				NPVariant *result);
		static bool _SetProperty(NPObject *npobj, NPIdentifier name,
				const NPVariant *value);
		static bool _RemoveProperty(NPObject *npobj, NPIdentifier name);
		static bool _Enumerate(NPObject *npobj, NPIdentifier **identifier,
				uint32_t *count);
		static bool _Construct(NPObject *npobj, const NPVariant *args,
				uint32_t argCount, NPVariant *result);
};
