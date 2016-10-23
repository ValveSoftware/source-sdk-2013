#pragma once

#include "cbase.h"
#include <functional>

class DLL_API IScriptingLanguage
{
	friend class CScriptManager;

public:

	IScriptingLanguage()			{};
	virtual ~IScriptingLanguage()	{};

	virtual void Initialize()=0;
	virtual void Terminate()=0;

	// Register a hook
	virtual void AddHook(const char* name)=0;
	// Call a  hook
	virtual bool CallHook(const char* name, ...) = 0;

	// Amount of memory in use (in bytes)
	virtual size_t GetMemoryUsage()=0;
};

