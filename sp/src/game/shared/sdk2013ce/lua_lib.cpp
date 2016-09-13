#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "lua_lib.h"

int _print(lua_State* state)
{
	//int args = lua_gettop(state);

	const char* msg = lua_tostring(state, 1);
	Msg(msg);

	return 0;
}

int _error(lua_State* state)
{
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	const char* message = lua_tostring(state, -1);
	Warning("[Lua] Error: %s\n", message);
	lua_pop(state, 1);

	return 0;
}

int _type(lua_State* state)
{
	return 0;
}