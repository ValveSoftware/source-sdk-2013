//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mapbase-specific user messages.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "usermessages.h"
#ifdef CLIENT_DLL
#include "hud_macros.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
void HookMapbaseUserMessages( void )
{
	// VScript
	//HOOK_MESSAGE( ScriptMsg ); // Hooked in CNetMsgScriptHelper
}
#endif

void RegisterMapbaseUserMessages( void )
{
	// VScript
	usermessages->Register( "ScriptMsg", -1 ); // CNetMsgScriptHelper

#ifdef CLIENT_DLL
	// TODO: Better placement?
	HookMapbaseUserMessages();
#endif
}
