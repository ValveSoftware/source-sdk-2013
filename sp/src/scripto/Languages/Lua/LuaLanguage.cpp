#include "cbase.h"
#include "LuaLanguage.h"

#define LUA_GLOBALNAME "_G"

int _lua_error(lua_State* L)
{
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	const char* message = luaL_tolstring(L, -1, NULL);

	ScriptError("[Script] Error: %s\n", message);

	return 0;
}

luaL_Reg libs[] = {
	//{ LUA_GLOBALNAME, luaopen_base },
	//{ LUA_LOADLIBNAME, luaopen_package },
	{ LUA_COLIBNAME, luaopen_coroutine },
	{ LUA_TABLIBNAME, luaopen_table },
	//{ LUA_IOLIBNAME, luaopen_io },
	//{ LUA_OSLIBNAME, luaopen_os },
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ LUA_UTF8LIBNAME, luaopen_utf8 },
	{ LUA_DBLIBNAME, luaopen_debug },
	{ NULL, NULL }
};

void CLuaLanguage::Initialize()
{
	L = luaL_newstate();
	lua_atpanic(L, _lua_error);

	for (int i = 0; i < sizeof(libs); i++)
	{
		ScriptLog("Adding Lib: %s", libs[i].name);

	}
}

void CLuaLanguage::Terminate()
{
	lua_close(L);
}


void CLuaLanguage::AddHook(const char* name)
{

}

void CLuaLanguage::CallHook(const char* name)
{
	lua_getglobal(L, name);

	// 0 args, multiple returns
	lua_call(L, 0, LUA_MULTRET);
}

size_t CLuaLanguage::GetMemoryUsage()
{
	// kb to b
	return lua_gc(L, LUA_GCCOUNT, NULL) * 1000;
}