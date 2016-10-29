#include "cbase.h"
#include "LuaLanguage.h"

#define LUA_GLOBALNAME "_G"

int _lua_error(lua_State* L)
{
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	const char* message = luaL_tolstring(L, -1, NULL);

	ScriptError(message);

	return 0;
}

template<typename F, typename R, typename... Args>
int _lua_call(lua_State* L, F func)
{
	
	return 1;
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
		// FIXME: This crahses the game
		//if (libs[i].name != NULL) {
			//ScriptLog("[Lua] Adding Lib: %s", libs[i].name);
		//}
	}
}

void CLuaLanguage::Terminate()
{
	lua_close(L);
}


void CLuaLanguage::AddHook(const char* name)
{

}

bool CLuaLanguage::CallHook(const char* name, ...)
{
	// FIXME: This crashes the game

	//lua_getglobal(L, name);

	// 0 args, 0 returns
	//lua_call(L, 0, 0);

	return true;
}

size_t CLuaLanguage::GetMemoryUsage()
{
	// kb to b
	return lua_gc(L, LUA_GCCOUNT, NULL) * 1000;
}