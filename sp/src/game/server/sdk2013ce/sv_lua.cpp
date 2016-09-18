#include "cbase.h"
#include "lua.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Server-Side Lua Stuff

void cc_lua_run(const CCommand& cmd)
{
	const char* str = cmd.ArgS();
	CLuaManager::doString(str);

}

ConCommand lua_run("lua_run", cc_lua_run, "", FCVAR_CHEAT);