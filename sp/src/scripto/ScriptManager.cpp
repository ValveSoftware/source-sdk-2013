#include "cbase.h"
#include "ScriptManager.h"

CScriptManager::CScriptManager()
{

}

void CScriptManager::Terminate()
{
	for (int i = 0; i < languages.Count(); i++) {
		languages[i]->Terminate();
	}
}

int CScriptManager::AddLanguage(IScriptingLanguage* language)
{
	languages.AddToTail(language);
	language->Initialize();
	return languages.Count();
}



void CScriptManager::AddHook(const char* name)
{
	ScriptLog("Added Hook: %s", name);

	hooks.AddToTail(name);
	for (int i = 0; i < languages.Count(); i++) {
		languages[i]->AddHook(name);
	}
}

bool CScriptManager::CallHook(const char* name, ...)
{
	// TODO: System to pass all vargs

	va_list args;
	va_start(args, name);

	bool ret = true;
	for (int i = 0; i < languages.Count(); i++) {
		ret &= languages[i]->CallHook(name, args);
	}
	
	va_end(args);
	
	return ret;
}