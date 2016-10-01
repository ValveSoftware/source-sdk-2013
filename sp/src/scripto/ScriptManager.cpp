#include "cbase.h"
#include "ScriptManager.h"

std::vector<IScriptingLanguage*> CScriptManager::languages;
std::vector<const char*> CScriptManager::hooks;

void CScriptManager::Terminate()
{
	for (unsigned int i = 0; i < languages.size(); i++) {
		languages[i]->Terminate();
	}
}

int CScriptManager::AddLanguage(IScriptingLanguage* language)
{
	languages.push_back(language);
	language->Initialize();

	return languages.size();
}

IScriptingLanguage* CScriptManager::GetLanguage(int id)
{
	return languages[id];
}

void CScriptManager::AddHook(const char* name)
{
	hooks.push_back(name);
	for (unsigned int i = 0; i < languages.size(); i++) {
		languages[i]->AddHook(name);
	}
}

void CScriptManager::CallHook(const char* name)
{
	for (unsigned int i = 0; i < languages.size(); i++) {
		languages[i]->CallHook(name);
	}
}