#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "scriptmanager.h"
#include "lua_lib.h"


int luaopen_overrides(lua_State* state)
{
	// Lower level stuff has to be
	// implemented in-engine (see lua_lib.h)
	// so first of all we add the base lib.
	// It gets to use the global namespace.
	lua_pushglobaltable(state);
	//luaL_setfuncs(state, lib_base, 0);

	// set _G
	lua_pushvalue(state, -1);
	lua_setfield(state, -2, "_G");

	// set _VERSION
	lua_pushliteral(state, LUA_VERSION);
	lua_setfield(state, -2, "_VERSION");

	// new way
	CScriptManager::BindFunction(_print, "print");
	CScriptManager::BindFunction(_error, "error");

	return 1;
}

// these functions do it the deprecated way

ScriptVariable_t* _print(ScriptVariable_t* args, int argc)
{
	for (int i = 1; i <= argc; i++) {
		ConColorMsg(COLOR_CYAN, args[i].ToString());
	}
	Msg("\n");
	
	return nullptr;
}

ScriptVariable_t* _error(ScriptVariable_t* args, int argc)
{
	// TODO: Implement

	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	//const char* message = luaL_tolstring(state, -1, NULL);

	//luaL_error(state, message);
	Warning("[Script] Error: %s\n");

	return nullptr;
}