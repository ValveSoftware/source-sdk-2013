#include "cbase.h"
#include "tier0/dbg.h"
#include "Filesystem.h"
#include "string.h"
#include "../../lua/lua.hpp"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "lua.h"

#pragma region Internal

void _Lua_error(lua_State* state)
{
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	const char* message = lua_tostring(state, -1);
	Warning("[Lua] Error: %s\n", message);
	lua_pop(state, 1);
}

int _Lua_print(lua_State* state)
{
	//int args = lua_gettop(state);
	
	const char* msg = lua_tostring(state, 1);
	Msg(msg);

	return 0;
}

#pragma endregion

void CLuaHandle::LoadFile(IFileSystem* filesystem, const char* path)
{
	DevMsg("[Lua] Loading %s\n", path);

	FileHandle_t f = filesystem->Open(path, "rb", "MOD");
	if (!f) {
		Warning("[Lua] Failed to load %s\n", path);
		return;
	}

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 1);

	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
	//Assert(buffer);

	((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f);
	buffer[fileSize] = '\0'; // null terminate file as EOF
	filesystem->Close(f);

	// Load the file
	int result = luaL_loadbuffer(state, buffer, fileSize, path);
	if (result != LUA_OK) {
		_Lua_error(state);
	}
}

void CLuaHandle::LoadDir(IFileSystem* filesystem, const char* dir)
{
	char searchPath[MAX_PATH];
	V_snprintf(searchPath, MAX_PATH, "%s/*", dir);
	V_FixDoubleSlashes(searchPath);
	V_FixSlashes(searchPath);

	DevMsg("[Lua] Loading directory %s\n", searchPath);

	FileFindHandle_t finder;
	const char* pFileName = filesystem->FindFirstEx(searchPath, "MOD", &finder);

	while (pFileName)
	{
		// pFileName:	test.lua
		// path:		lua/test.lua
		char path[MAX_PATH];
		V_snprintf(path, MAX_PATH, "%s/%s", dir, pFileName);
		V_FixSlashes(path);

		if (filesystem->IsDirectory(path, "MOD")) {
			if (pFileName[0] != '.') {
				LoadDir(filesystem, path);
			}
		} else {
			LoadFile(filesystem, path);
		}

		pFileName = filesystem->FindNext(finder);
	}

	filesystem->FindClose(finder);
}

CLuaHandle::CLuaHandle(IFileSystem* filesystem)
{
	Msg("[Lua] Initializing\n");

	state = luaL_newstate();
	
	//luaL_openlibs(state);
	lua_register(state, "print", _Lua_print);

	LoadDir(filesystem, "lua");
	// TODO: Execute all files.
}

CLuaHandle::~CLuaHandle()
{
	Msg("[Lua] Shutting down.\n");
	lua_close(state);
}