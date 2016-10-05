// cbase.cpp : source file that includes just the standard includes
// Scripto.pch will be the pre-compiled header
// cbase.obj will contain the pre-compiled type information

#include "cbase.h"

#ifdef SOURCE_ENGINE

void ScriptLog(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	tchar buffer[512];
	if (vsnprintf(buffer, sizeof(buffer), msg, args) < 0)
		return;

	GetSpewOutputFunc()(SPEW_LOG, buffer);
	
	va_end(args);
}

void ScriptError(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	tchar buffer[512];
	if (vsnprintf(buffer, sizeof(buffer), msg, args) < 0)
		return;

	GetSpewOutputFunc()(SPEW_WARNING, buffer);

	va_end(args);
}

#endif