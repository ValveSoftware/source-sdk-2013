#ifndef LUA_H
#define LUA_H

#ifdef _WIN32
#pragma once
#endif

#include "..\..\lua\lua.hpp"

// All Lua functions are declared the same way.
#define LUA_FUNCTION(name)	int name(lua_State* state)

#define LUA_ARGS(argc)		int argc = lua_gettop(state)
#define LUA_ARGS_MIN(argc, minArgs)		\
	int argc = lua_gettop(state);		\
	luaL_argcheck(state, argc >= minArgs, 1, "")

class CLuaManager
{
private:
	CLuaManager();
	~CLuaManager();

	static lua_State* state;
public:
	static void init(IFileSystem* filesystem);
	static void close();

	// Load a file relative to the 'MOD' directory.
	static void loadFile(IFileSystem* pFilesystem, const char* filename);
	// Recursively load every .lua file in a folder.
	static void loadDir(IFileSystem* pFilesystem, const char* dirname);

	// Executes str.
	static void doString(const char* str);

	// Calls hook hookName.
	static void call(const char* hookName);
};

#endif