#include "cbase.h"
#include "tier0/dbg.h"
#include "Filesystem.h"
#include "string.h"
#include "../../lua/lua.hpp"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "lua.h"
#include "lua_lib.h"

#include <functional>

// Wrap calls to lua_pcall etc in this.
// TODO: Convert lua_State to ScriptVariable_t
#define LUA_CATCH(x) if ((x) != LUA_OK) {}

#pragma region Internal

int _lua_error(lua_State* state)
{
	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	const char* message = luaL_tolstring(state, -1, NULL);

	//luaL_error(state, message);
	Warning("[Script] Error: %s\n", message);

	return 0;
}

template<typename Callable>
int _lua_call(lua_State* state, Callable* pFunc)
{

}


#pragma endregion



void CLuaManager::LoadFile(IFileSystem* filesystem, const char* path)
{
	ConDColorMsg(COLOR_CYAN, "[Lua] Loading %s\n", path);

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
	LUA_CATCH (
		luaL_loadbuffer(state, buffer, fileSize, path)
	)

	// TODO: Don't do this here.
	LUA_CATCH (
		lua_pcall(state, 0, LUA_MULTRET, 0) 
	)
}

void CLuaManager::LoadDir(IFileSystem* filesystem, const char* dir)
{
	char searchPath[MAX_PATH];
	V_snprintf(searchPath, MAX_PATH, "%s/*", dir);
	V_FixDoubleSlashes(searchPath);
	V_FixSlashes(searchPath);

	ConDColorMsg(COLOR_CYAN, "[Lua] Loading directory %s\n", searchPath);

	FileFindHandle_t finder;
	const char* pFileName = filesystem->FindFirstEx(searchPath, "MOD", &finder);

	while (pFileName)
	{
		//		   Sample:
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

void CLuaManager::DoFile(IFileSystem* pFilesystem, const char* filename)
{

}


void CLuaManager::DoString(const char* str)
{
	ConDColorMsg(COLOR_CYAN, "[Lua] Running String: %s\n", str);

	LUA_CATCH (
		luaL_dostring(state, str)
	)
}

void CLuaManager::Call(const char* hook)
{
	lua_getglobal(state, hook);
	LUA_CATCH(lua_pcall(state, 0, 0, 0));
}


template<typename F>
void CLuaManager::BindFunction(const char* funcName, F func)
{
	using namespace std::placeholders;
	// TODO Convert func to a std::function
	std::function pFunc;
	// Bind a lua_CFunction: cfunc(state) = _lua_call(state, pFunc)
	auto cfunc = std::bind(_lua_call<std::function>, _1, &pFunc);
	// Register the lua_CFunction
	lua_register(state, funcName, cfunc);

	functionBinds.AddToTail({ funcName, cfunc });
	
}

CUtlVector<const char*>* CLuaManager::GetBinds()
{
	int len = functionBinds.Count();

	CUtlVector<const char*>* bindsList = new CUtlVector<const char*>();
	for (int i = 0; i < len; i++)
	{
		bindsList->AddToTail(functionBinds[i].name);
	}

	return bindsList;
}

CUtlVector<const char*>* CLuaManager::GetHooks()
{
	return &hooks;
}

void CLuaManager::Init(IFileSystem* filesystem)
{
	ConColorMsg(COLOR_CYAN, "[Lua] Initializing\n");
	
	state = luaL_newstate();
	//lua_atpanic(state, _error);

	AddLibs();

	// Paths and hook names are currently hardcoded
	LoadDir(filesystem, "scripts/lua/init");
}

const luaL_Reg lua_libs[] = {
	{ "_G", luaopen_base },
	//{ LUA_LOADLIBNAME, luaopen_package },
	{ LUA_COLIBNAME, luaopen_coroutine },
	{ LUA_TABLIBNAME, luaopen_table },
	//{ LUA_IOLIBNAME, luaopen_io },
	{ LUA_OSLIBNAME, luaopen_os },
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ LUA_UTF8LIBNAME, luaopen_utf8 },
	{ LUA_DBLIBNAME, luaopen_debug },
	{ NULL, NULL }
};

void CLuaManager::AddLibs()
{
	// Load Libraries

	//const luaL_Reg *lib;

	const luaL_Reg *lib;
	/* "require" functions from 'loadedlibs' and set results to global table */
	for (lib = lua_libs; lib->func; lib++) {
		ConDColorMsg(COLOR_CYAN, "[Lua] Loading: %s\n", lib->name);
		luaL_requiref(state, lib->name, lib->func, 1);
		lua_pop(state, 1);  /* remove lib */
	}
	
	luaopen_overrides(state);
}

void CLuaManager::Close()
{
	ConColorMsg(COLOR_CYAN, "[Lua] Shutting down.\n");
	lua_close(state);
}