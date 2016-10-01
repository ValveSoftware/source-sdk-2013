#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "IScriptingLanguage.h"



// static class to manage all scripts
class CScriptManager
{
private:
	CScriptManager()	{};
	~CScriptManager()	{};

public:
	// TODO make this a proper array of languages or something
	static IScriptingLanguage* lang;

	static void AddHook(const char* hookname);
	
	// Binds the C function func to script function funcname
	template<typename F, typename... Args>
	static void BindFunction(F function, const char* funcName);

	static void Init(IFileSystem* filesystem);

	static void Close() {
		lang->Close();
	};

	// Call a scripting hook.
	// TODO: Add internal indexes so we can
	// make hook calling 100x faster.
	// i.e. catch when hook functions aren't defined,
	// store function addresses in wam.
	static int Call(const char* name, ...) {
		// TODO Args
		lang->Call(name);
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

	virtual ~CScriptConCommand(void) {};

	bool IsCommand(void) const { return true; };

	// TODO
	//int AutoCompleteSuggest(const char *partial, CUtlVector< CUtlString > &commands);

	bool CanAutoComplete(void) { return false; };

	// Invoke the function
	void Dispatch(const CCommand &command);

};

#endif