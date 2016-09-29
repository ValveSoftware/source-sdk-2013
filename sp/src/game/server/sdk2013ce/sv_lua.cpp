#include "cbase.h"
#include "lua.h"
#include "scriptmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Server-Side Lua Stuff

void cc_lua_run(const CCommand& cmd)
{
	const char* str = cmd.ArgS();
	CScriptManager::lang->DoString(str);
}

ConCommand lua_run("lua_run", cc_lua_run, "", FCVAR_CHEAT);

void cc_lua_binds(const CCommand& cmd)
{
	CUtlVector<const char*>* list = CScriptManager::lang->GetBinds();
	for (int i = 0; i < list->Count(); i++)
	{
		ConColorMsg(COLOR_CYAN, "[Lua] %s\n", list->Element(i));
	}
}

ConCommand lua_binds("lua_binds", cc_lua_binds, "");

void cc_lua_hooks(const CCommand& cmd)
{
	CUtlVector<const char*>* list = CScriptManager::lang->GetHooks();
	for (int i = 0; i < list->Count(); i++)
	{
		ConColorMsg(COLOR_CYAN, "[Lua] %s\n", list->Element(i));
	}
}

ConCommand lua_hooks("lua_hooks", cc_lua_hooks, "");