#pragma once

#include "lua/lua.hpp"
#include "../../IScriptingLanguage.h"

class DLL_API CLuaLanguage : public IScriptingLanguage
{
private:

	lua_State* L;
public:
	CLuaLanguage()	{};
	~CLuaLanguage()	{};

	void Initialize();
	void Terminate();

	// Register a hook
	void AddHook(const char* name);
	// Call a  hook
	bool CallHook(const char* name, ...);

	size_t GetMemoryUsage();
};

