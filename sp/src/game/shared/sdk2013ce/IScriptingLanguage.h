#ifndef SCRIPTING_LANGUAGE_H
#define SCRIPTING_LANGUAGE_H

#ifdef _WIN32
#pragma once
#endif

struct ScriptObject_t {
	// TODO
};

union ScriptVariable_t {
	const char* string;
	int integer;
	float number;
	ScriptObject_t object;

	ScriptVariable_t()
	{

	}

	ScriptVariable_t(const char* str)
	{
		string = str;
	}

	const char* ToString()
	{
		// TODO
		return nullptr;
	}
};

// C++ function that can be bound to a script function
typedef ScriptVariable_t* (*BindFunction_t)(ScriptVariable_t* args, int argc);

// Handle for calling a script function from C++
class CScriptFunction
{
private:
public:
	int argc = 0;

	CScriptFunction();
	~CScriptFunction();

	// length of args must == argc
	void Call(ScriptVariable_t* args);
};

class IScriptingLanguage
{
public:
	virtual void Init(IFileSystem* filesystem)=0;
	virtual void Close()=0;
	//virtual void Error();

	virtual void DoFile(IFileSystem* pFilesystem, const char* filename)=0;
	virtual void DoString(const char* string)=0;

	virtual void AddHook(const char* func) = 0;
	virtual void Call(const char* func)=0;
	virtual CUtlVector<const char*>* GetHooks() = 0;

	virtual void BindFunction(BindFunction_t func, const char* funcName)=0;
	virtual CUtlVector<const char*>* GetBinds() = 0;

	
};

#endif