//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( HUD_MACROS_H )
#define HUD_MACROS_H
#ifdef _WIN32
#pragma once
#endif

#include "usermessages.h"

// Macros to hook function calls into the HUD object
#define HOOK_MESSAGE(x) usermessages->HookMessage(#x, __MsgFunc_##x );
#define HOOK_HUD_MESSAGE(y, x) usermessages->HookMessage(#x, __MsgFunc_##y##_##x );
// Message declaration for non-CHudElement classes
#define DECLARE_MESSAGE(y, x) void __MsgFunc_##y##_##x(bf_read &msg) \
	{											\
		y.MsgFunc_##x( msg );	\
	}
// Message declaration for CHudElement classes that use the hud element factory for creation
#define DECLARE_HUD_MESSAGE(y, x) void __MsgFunc_##y##_##x(bf_read &msg) \
	{																\
		CHudElement *pElement = gHUD.FindElement( #y );				\
		if ( pElement )												\
		{															\
			((y *)pElement)->MsgFunc_##x( msg );	\
		}															\
	}

#define DECLARE_HUD_MESSAGE_BASECLASS(name, basename, msgname) void __MsgFunc_##msgname(const char *pszName, int iSize, void *pbuf) \
	{																\
		CHudElement *pElement = gHUD.FindElement( #name );				\
		if ( pElement )												\
		{															\
			((basename *)pElement)->MsgFunc_##msgname(pszName, iSize, pbuf );	\
		}															\
	}

// Commands
#define HOOK_COMMAND(x, y) static ConCommand x( #x, __CmdFunc_##y, "", FCVAR_SERVER_CAN_EXECUTE );
// Command declaration for non CHudElement classes
#define DECLARE_COMMAND(y, x) void __CmdFunc_##x( void ) \
	{							\
		y.UserCmd_##x( );		\
	}
// Command declaration for CHudElement classes that use the hud element factory for creation
#define DECLARE_HUD_COMMAND(y, x) void __CmdFunc_##x( void )									\
	{																\
		CHudElement *pElement = gHUD.FindElement( #y );				\
		{															\
			((y *)pElement)->UserCmd_##x( );						\
		}															\
	}

#define DECLARE_HUD_COMMAND_NAME(y, x, name) void __CmdFunc_##x( void )									\
	{																\
		CHudElement *pElement = gHUD.FindElement( name );			\
		{															\
			((y *)pElement)->UserCmd_##x( );						\
		}															\
	}


#endif // HUD_MACROS_H
