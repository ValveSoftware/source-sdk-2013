#include "cbase.h"
#include "scriptmanager.h"


#include "keyvalues.h"


void CScriptManager::AddHook(const char* HookName)
{
	// TODO!!
}


void CScriptManager::BindFunction(BindFunction_t function, const char* funcName)
{
	// TODO!!
}

CScriptConCommand::CScriptConCommand(const char *pName, CScriptFunction* callback, const char *pHelpString = 0, int flags = 0)
{
	BaseClass::Create(pName, pHelpString, flags);
	pCallback = callback;
}

void CScriptConCommand::Dispatch(const CCommand &cmd)
{
	//pCallback->Call
	// TODO!!
}
