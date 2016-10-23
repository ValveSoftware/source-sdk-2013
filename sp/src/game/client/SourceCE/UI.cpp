#include "cbase.h"
#include "UI.h"

#include <tinyxml2/tinyxml2.h>

void UIMsg(const char* msg, ...)
{
	char buf[256];

	va_list args;
	va_start(args, msg);

	vsprintf(buf, msg, args);

	va_end(args);

	//Msg("[UI] %s\n", buf);
	ConColorMsg(COLOR_ORANGE, "[UI] %s\n", buf);
}

void UIDevMsg(const char* msg, ...)
{
	char buf[256];

	va_list args;
	va_start(args, msg);

	vsprintf(buf, msg, args);

	va_end(args);

	//Msg("[UI] %s\n", buf);
	ConDColorMsg(COLOR_ORANGE, "[UI] %s\n", buf);
}

void UIError(const char* msg, ...)
{
	char buf[256];

	va_list args;
	va_start(args, msg);

	vsprintf(buf, msg, args);

	va_end(args);

	//Msg("[UI] %s\n", buf);
	ConColorMsg(COLOR_ORANGE, "[UI] Error: %s\n", buf);
}

bool CycloramaUI::Init()
{
	return true;
}

void CycloramaUI::Shutdown()
{

}