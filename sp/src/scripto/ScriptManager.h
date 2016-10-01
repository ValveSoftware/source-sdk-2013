
#pragma once

#include "cbase.h"

#include "IScriptingLanguage.h"

template class DLL_API std::vector < IScriptingLanguage* >;
template class DLL_API std::vector < const char* >;

// (static) CScriptManager
class DLL_API CScriptManager
{
private:
	CScriptManager()	{};
	~CScriptManager()	{};

	static std::vector<IScriptingLanguage*> languages;

	static std::vector<const char*> hooks;
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
	static void CallHook(const char* name);
	// Get a list of all hook names
	inline static std::vector<const char*> GetHooks() { return hooks; };

	///////////////////////////////////////////////////////////
	// Binds:
	//	C++ functions that can be called from scripts
	///////////////////////////////////////////////////////////

	//template<typename F>
	//static void AddBind(const char* name, F func);
	
	//static std::vector<const char*> GetBinds();
	
};