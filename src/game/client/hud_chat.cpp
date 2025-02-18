//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hud_chat.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "hud_basechat.h"
#include <vgui/ILocalize.h>



DECLARE_HUDELEMENT( CHudChat );

DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );

//=====================
//CHudChat
//=====================

CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
	
}

void CHudChat::Init( void )
{
	BaseClass::Init();

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
}

//-----------------------------------------------------------------------------
// Purpose: Reads in a player's Chat text from the server
//-----------------------------------------------------------------------------
void CHudChat::MsgFunc_SayText2( bf_read &msg )
{
	int client = msg.ReadByte();
	bool bWantsToChat = msg.ReadByte();

	wchar_t szBuf[6][256];
	char untranslated_msg_text[256];
	wchar_t *msg_text = ReadLocalizedString( msg, szBuf[0], sizeof( szBuf[0] ), false, untranslated_msg_text, sizeof( untranslated_msg_text ) );

	// keep reading strings and using C format strings for subsituting the strings into the localised text string
	ReadChatTextString ( msg, szBuf[1], sizeof( szBuf[1] ) );		// player name
	ReadChatTextString ( msg, szBuf[2], sizeof( szBuf[2] ) );		// chat text
	ReadLocalizedString( msg, szBuf[3], sizeof( szBuf[3] ), true );
	ReadLocalizedString( msg, szBuf[4], sizeof( szBuf[4] ), true );

	g_pVGuiLocalize->ConstructString( szBuf[5], sizeof( szBuf[5] ), msg_text, 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );

	char ansiString[512];
	g_pVGuiLocalize->ConvertUnicodeToANSI( ConvertCRtoNL( szBuf[5] ), ansiString, sizeof( ansiString ) );

	if ( bWantsToChat )
	{
		// print raw chat text
		ChatPrintf( client, CHAT_FILTER_NONE, "%s", ansiString );

		Msg( "%s\n", RemoveColorMarkup(ansiString) );
	}
	else
	{
		// print raw chat text
		ChatPrintf( client, CHAT_FILTER_NONE, "%s", ansiString );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CHudChat::MsgFunc_SayText( bf_read &msg )
{
	char szString[256];

	msg.ReadByte(); // client ID
	msg.ReadString( szString, sizeof(szString) );
	Printf( CHAT_FILTER_NONE, "%s", szString );
}


// Message handler for text messages
// displays a string, looking them up from the titles.txt file, which can be localised
// parameters:
//   byte:   message direction  ( HUD_PRINTCONSOLE, HUD_PRINTNOTIFY, HUD_PRINTCENTER, HUD_PRINTTALK )
//   string: message 
// optional parameters:
//   string: message parameter 1
//   string: message parameter 2
//   string: message parameter 3
//   string: message parameter 4
// any string that starts with the character '#' is a message name, and is used to look up the real message in titles.txt
// the next (optional) one to four strings are parameters for that string (which can also be message names if they begin with '#')
void CHudChat::MsgFunc_TextMsg( bf_read &msg )
{
	char szString[2048];
	int msg_dest = msg.ReadByte();
	static char szBuf[6][256];

	msg.ReadString( szString, sizeof(szString) );
	char *msg_text = hudtextmessage->LookupString( szString, &msg_dest );
	Q_strncpy( szBuf[0], msg_text, sizeof( szBuf[0] ) );
	msg_text = szBuf[0];

	// keep reading strings and using C format strings for subsituting the strings into the localised text string
	msg.ReadString( szString, sizeof(szString) );
	char *sstr1 = hudtextmessage->LookupString( szString );
	Q_strncpy( szBuf[1], sstr1, sizeof( szBuf[1] ) );
	sstr1 = szBuf[1];

	StripEndNewlineFromString( sstr1 );  // these strings are meant for subsitution into the main strings, so cull the automatic end newlines
	msg.ReadString( szString, sizeof(szString) );
	char *sstr2 = hudtextmessage->LookupString( szString );
	Q_strncpy( szBuf[2], sstr2, sizeof( szBuf[2] ) );
	sstr2 = szBuf[2];
	
	StripEndNewlineFromString( sstr2 );
	msg.ReadString( szString, sizeof(szString) );
	char *sstr3 = hudtextmessage->LookupString( szString );
	Q_strncpy( szBuf[3], sstr3, sizeof( szBuf[3] ) );
	sstr3 = szBuf[3];

	StripEndNewlineFromString( sstr3 );
	msg.ReadString( szString, sizeof(szString) );
	char *sstr4 = hudtextmessage->LookupString( szString );
	Q_strncpy( szBuf[4], sstr4, sizeof( szBuf[4] ) );
	sstr4 = szBuf[4];
	
	StripEndNewlineFromString( sstr4 );
	char *psz = szBuf[5];

	if ( !cl_showtextmsg.GetInt() )
		return;

	switch ( msg_dest )
	{
	case HUD_PRINTCENTER:
		Q_snprintf( psz, sizeof( szBuf[5] ), msg_text, sstr1, sstr2, sstr3, sstr4 );
		internalCenterPrint->Print( ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTNOTIFY:
		psz[0] = 1;  // mark this message to go into the notify buffer
		Q_snprintf( psz+1, sizeof( szBuf[5] ) - 1, msg_text, sstr1, sstr2, sstr3, sstr4 );
		Msg( "%s", ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTTALK:
		Q_snprintf( psz, sizeof( szBuf[5] ), msg_text, sstr1, sstr2, sstr3, sstr4 );
		Printf( CHAT_FILTER_NONE, "%s", ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTCONSOLE:
		Q_snprintf( psz, sizeof( szBuf[5] ), msg_text, sstr1, sstr2, sstr3, sstr4 );
		Msg( "%s", ConvertCRtoNL( psz ) );
		break;
	}
}

