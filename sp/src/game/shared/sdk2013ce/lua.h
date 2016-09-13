#ifndef LUA_H
#define LUA_H

#ifdef _WIN32
#pragma once
#endif

#include "..\..\lua\lua.hpp"

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