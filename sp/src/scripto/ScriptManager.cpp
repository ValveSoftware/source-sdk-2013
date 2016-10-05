#include "cbase.h"
#include "ScriptManager.h"

CUtlVector<IScriptingLanguage*> CScriptManager::languages;
CUtlVector<const char*> CScriptManager::hooks;

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
	hooks.AddToTail(name);
	for (int i = 0; i < languages.Count(); i++) {
		languages[i]->AddHook(name);
	}
}

int CScriptManager::CallHook(const char* name, ...)
{
	for (int i = 0; i < languages.Count(); i++) {
		languages[i]->CallHook(name);
	}
	return 1;
}