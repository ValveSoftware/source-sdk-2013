#pragma once
class IScriptingLanguage
{
private:
	IScriptingLanguage()	{};
	~IScriptingLanguage()	{};

public:
	static void Init(IFileSystem* filesystem);
	static void Close();

	static void DoFile(IFileSystem* pFilesystem, const char* filename);
	static void DoString(const char* string);

	static void Call(const char* func);
};

