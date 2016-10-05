
#pragma once

#include "cbase.h"

#include "IScriptingLanguage.h"

template class DLL_API CUtlMemory < IScriptingLanguage* >;
template class DLL_API CUtlMemory < const char* >;

template class DLL_API CUtlVector < IScriptingLanguage* >;
template class DLL_API CUtlVector < const char* >;

// (static) CScriptManager
class DLL_API CScriptManager
{
private:
	CScriptManager()	{};
	~CScriptManager()	{};

	static CUtlVector<IScriptingLanguage*> languages;

	static CUtlVector<const char*> hooks;
public:

	// Add an IScriptingLanguage to the script manager
	// and return the language's internal ID
	static int AddLanguage(IScriptingLanguage* language);
	
	// Gets the IScriptingLanguage at ID for direct use
	inline static IScriptingLanguage* GetLanguage(int id) { return languages[id]; };

	// Destroys the script engine.
	static void Terminate();

	///////////////////////////////////////////////////////////
	// Hooks:
	//	Script functions that can be called from C++
	///////////////////////////////////////////////////////////

	// Register a hook
	static void AddHook(const char* name);
	// Call a  hook
	// TODO: Proper return types and args
	static int CallHook(const char* name, ...);
	// Get a list of all hook names
	static CUtlVector<const char*>* GetHooks() { return &hooks; };

	///////////////////////////////////////////////////////////
	// Binds:
	//	C++ functions that can be called from scripts
	///////////////////////////////////////////////////////////

	//template<typename F>
	//static void AddBind(const char* name, F func);
	
	//static CUtlVector<const char*> GetBinds();
	
};