#include "cbase.h"
#include "scriptmanager.h"


#include "keyvalues.h"


void CScriptManager::AddHook(const char* HookName)
{

}

CScriptConCommand::CScriptConCommand(const char *pName, const char *pHelpString = 0, int flags = 0)
{
	BaseClass::Create(pName, pHelpString, flags);
}

void CScriptConCommand::Dispatch(const CCommand &cmd)
{
	// TODO
}
