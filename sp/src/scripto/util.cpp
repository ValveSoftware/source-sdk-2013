#include "cbase.h"
#include "util.h"

void ScriptLog(const char* msg, ...)
{
	char buf[256];

	va_list args;
	va_start(args, msg);
	
	vsprintf(buf, msg, args);

	va_end(args);

	//Msg("[Script] %s\n", buf);
	ConColorMsg(COLOR_CYAN, "[Script] %s\n", buf);
}

void ScriptError(const char* msg, ...)
{
	char buf[256];

	va_list args;
	va_start(args, msg);

	vsprintf(buf, msg, args);

	va_end(args);

	//Msg("[Script] Error: %s\n", buf);
	ConColorMsg(COLOR_RED, "[Script] %s\n", buf);
}