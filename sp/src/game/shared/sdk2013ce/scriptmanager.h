#pragma once
// Currently just inlined calls to CLuaManager.
// To be replaced later with an actual script
// manager somewhat like VScript.

#include "lua.h"

class CScriptManager
{
private:
	CScriptManager()	{};
	~CScriptManager()	{};

	

public:
	static void AddHook(const char* hookname);

	static inline void Init(IFileSystem* filesystem) {
		CLuaManager::init(filesystem);
		CScriptConCommand cc_TEST("cc_test");
		Msg("CC_TEST: %d", cc_TEST.IsCommand());

		
	};
	static inline void Close() {
		CLuaManager::close();
	};

	// Call a scripting hook.
	// TODO: Add internal indexes so we can
	// make hook calling 100x faster.
	// i.e. catch when hook functions aren't defined,
	// store function addresses in wam.
	static inline int Call(const char* name, ...) {
		// TODO Pass args and return value.
		CLuaManager::call(name);
		return true;
	};
};

class CScriptConCommand : ConCommandBase
{
public:
	typedef ConCommandBase BaseClass;

	CScriptConCommand(const char *pName, const char *pHelpString = 0, int flags = 0);

	virtual ~CScriptConCommand(void);

	virtual	bool IsCommand(void) const { return true; };

	// TODO
	//virtual int AutoCompleteSuggest(const char *partial, CUtlVector< CUtlString > &commands);

	virtual bool CanAutoComplete(void) { return false; };

	// Invoke the function
	virtual void Dispatch(const CCommand &command);
};