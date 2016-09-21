#pragma once
#include "lua.h"


struct ScriptObject_t {
	// TODO
};

union ScriptVariable_t {
	const char* string;
	int integer;
	float number;
	ScriptObject_t object;

	const char* ToString()
	{
		// TODO
	}
};

// C++ function that can be bound to a script function
typedef ScriptVariable_t* (*BindFunction_t)(ScriptVariable_t* args, int argc);

// Handle for calling a script function from C++
class CScriptFunction
{
private:
public:
	int argc = 0;

	CScriptFunction();
	~CScriptFunction();

	// length of args must == argc
	void Call(ScriptVariable_t* args);
};

// static class to manage all scripts
class CScriptManager
{
private:
	CScriptManager()	{};
	~CScriptManager()	{};

public:
	static void AddHook(const char* hookname);
	
	// Binds the C function func to script function funcname
	static void BindFunction(BindFunction_t func, const char* funcName);

	static inline void Init(IFileSystem* filesystem) {
		// TODO: Placeholder
		CLuaManager::init(filesystem);
		
	};
	static inline void Close() {
		// TODO: Placeholder
		CLuaManager::close();
	};

	// Call a scripting hook.
	// TODO: Add internal indexes so we can
	// make hook calling 100x faster.
	// i.e. catch when hook functions aren't defined,
	// store function addresses in wam.
	static inline int Call(const char* name, ...) {
		// TODO Pass args and return value.
		// TODO: Placeholder
		CLuaManager::call(name);
		return true;
	};
};

class CScriptConCommand : ConCommandBase
{
private:
	CScriptFunction* pCallback;
public:
	typedef ConCommandBase BaseClass;

	CScriptConCommand(const char *pName, CScriptFunction* callback, const char *pHelpString = 0, int flags = 0);

	virtual ~CScriptConCommand(void);

	bool IsCommand(void) const { return true; };

	// TODO
	//int AutoCompleteSuggest(const char *partial, CUtlVector< CUtlString > &commands);

	bool CanAutoComplete(void) { return false; };

	// Invoke the function
	void Dispatch(const CCommand &command);

};