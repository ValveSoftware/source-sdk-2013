#pragma once
// Currently just inlined calls to CLuaManager.
// To be replaced later with an actual script
// manager somewhat like VScript.

#include "lua.h"
class CScriptManager
{
private:
	CScriptManager();
	~CScriptManager();

public:
	static inline void Init(IFileSystem* filesystem) {
		CLuaManager::init(filesystem);
	};
	static inline void Close() {
		CLuaManager::close();
	};

	// Call a scripting hook.
	// TODO: Add internal indexes so we can
	// make hook calling 100x faster.
	// i.e. catch when hook functions aren't defined,
	// store function addresses in wam.
	static inline void Call(const char* name) {
		CLuaManager::call(name);
	};
};

