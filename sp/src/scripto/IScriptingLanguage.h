#pragma once

#include "cbase.h"

class DLL_API IScriptingLanguage
{
	friend class CScriptManager;
private:
	IScriptingLanguage()	{};
	~IScriptingLanguage()	{};

public:
	virtual void Initialize()=0;
	virtual void Terminate()=0;

	// Register a hook
	virtual void AddHook(const char* name)=0;
	// Call a  hook
	virtual void CallHook(const char* name)=0;

	//template<typename F>
	//virtual void AddBind(const char* name, F func);
};

