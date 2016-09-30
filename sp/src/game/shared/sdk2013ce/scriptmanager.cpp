#include "cbase.h"
#include "lua.h"

#include "keyvalues.h"
#include "scriptmanager.h"

#include <functional>

IScriptingLanguage* CScriptManager::lang = NULL;

void CScriptFunction::Call(ScriptVariable_t* args)
{
	// TODO!!
}


void CScriptManager::Init(IFileSystem* filesystem)
{
	lang = new CLuaManager(filesystem);
}

void CScriptManager::AddHook(const char* HookName)
{
	lang->AddHook(HookName);
}

template<typename F, typename... Args>
void CScriptManager::BindFunction(F function, const char* funcName)
{
	lang->BindFunction(funcName, function);
}

CScriptConCommand::CScriptConCommand(const char *pName, CScriptFunction* callback, const char *pHelpString /*= 0*/, int flags /*= 0*/)
{
	BaseClass::Create(pName, pHelpString, flags);
	pCallback = callback;
}

void CScriptConCommand::Dispatch(const CCommand &cmd)
{
	ScriptVariable_t* args = new ScriptVariable_t[pCallback->argc];
	for (int i = 0; i < pCallback->argc; i++)
	{
		args[i] = cmd.Arg(i);
	}

	pCallback->Call(args);
}
