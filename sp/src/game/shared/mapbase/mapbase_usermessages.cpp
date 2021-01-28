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
void __MsgFunc_CallClientScriptFunction( bf_read &msg )
{
	char szFunction[64];
	if (!msg.ReadString( szFunction, sizeof( szFunction ) ))
	{
		CGMsg( 0, CON_GROUP_VSCRIPT, "Unable to read function string\n" );
	}

	int idx = msg.ReadByte();
	C_BaseEntity *pEntity = CBaseEntity::Instance( idx );

	if (pEntity)
	{
		if (pEntity->m_ScriptScope.IsInitialized())
		{
			//CGMsg( 0, CON_GROUP_VSCRIPT, "%s calling function \"%s\"\n", pEntity->GetDebugName(), szFunction );
			pEntity->CallScriptFunction( szFunction, NULL );
		}
		else
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s scope not initialized\n", pEntity->GetDebugName() );
		}
	}
	else
	{
		CGMsg( 0, CON_GROUP_VSCRIPT, "Clientside entity not found for script function (index %i)\n", idx );
	}
}

void HookMapbaseUserMessages( void )
{
	// VScript
	HOOK_MESSAGE( CallClientScriptFunction );
	//HOOK_MESSAGE( ScriptMsg ); // Hooked in CNetMsgScriptHelper
}
#endif

void RegisterMapbaseUserMessages( void )
{
	// VScript
	usermessages->Register( "CallClientScriptFunction", -1 );
	usermessages->Register( "ScriptMsg", -1 ); // CNetMsgScriptHelper

#ifdef CLIENT_DLL
	// TODO: Better placement?
	HookMapbaseUserMessages();
#endif
}
