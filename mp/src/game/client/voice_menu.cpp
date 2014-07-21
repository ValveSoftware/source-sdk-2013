//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "c_baseplayer.h"
#include "menu.h"
#include "KeyValues.h"
#include "multiplay_gamerules.h"

static int g_ActiveVoiceMenu = 0;

void OpenVoiceMenu( int index )
{
	// do not show the menu if the player is dead or is an observer
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() || pPlayer->IsObserver() )
		return;

	CHudMenu *pMenu = (CHudMenu *) gHUD.FindElement( "CHudMenu" );
	if ( !pMenu )
		return;

	// if they hit the key again, close the menu
	if ( g_ActiveVoiceMenu == index )
	{
		if ( pMenu->IsMenuOpen() )
		{
			pMenu->HideMenu();
			g_ActiveVoiceMenu = 0;
			return;
		}
	}

	if ( index > 0 && index < 9 )
	{
		KeyValues *pKV = new KeyValues( "MenuItems" );

		CMultiplayRules *pRules = dynamic_cast< CMultiplayRules * >( GameRules() );
		if ( pRules )
		{			
			if ( !pRules->GetVoiceMenuLabels( index-1, pKV ) )
			{ 
				pKV->deleteThis();
				return;
			}
		}

		pMenu->ShowMenu_KeyValueItems( pKV );

		pKV->deleteThis();

		g_ActiveVoiceMenu = index;
	}
	else
	{
		g_ActiveVoiceMenu = 0;
	}
}

static void OpenVoiceMenu_1( void )
{
	OpenVoiceMenu( 1 );
}

static void OpenVoiceMenu_2( void )
{
	OpenVoiceMenu( 2 );
}

static void OpenVoiceMenu_3( void )
{
	OpenVoiceMenu( 3 );
}

ConCommand voice_menu_1( "voice_menu_1", OpenVoiceMenu_1, "Opens voice menu 1" );
ConCommand voice_menu_2( "voice_menu_2", OpenVoiceMenu_2, "Opens voice menu 2" );
ConCommand voice_menu_3( "voice_menu_3", OpenVoiceMenu_3, "Opens voice menu 3" );

CON_COMMAND( menuselect, "menuselect" )
{
	if ( args.ArgC() < 2 )
		return;

	if( g_ActiveVoiceMenu == 0 )
	{
		// if we didn't have a menu open, maybe a plugin did.  send it on to the server.
		const char *cmd = VarArgs( "menuselect %s", args[1] );
		engine->ServerCmd( cmd );
		return;
	}

	int iSelection = atoi( args[ 1 ] );

	switch( g_ActiveVoiceMenu )
	{
	case 1:
	case 2:
	case 3:
		{
			char cmd[128];
			Q_snprintf( cmd, sizeof(cmd), "voicemenu %d %d", g_ActiveVoiceMenu - 1, iSelection - 1 );
			engine->ServerCmd( cmd );
		}
		break;

	default:
		{
			// if we didn't have a menu open, maybe a plugin did.  send it on to the server.
			const char *cmd = VarArgs( "menuselect %d", iSelection );
			engine->ServerCmd( cmd );
		}
		break;
	}

	// reset menu
	g_ActiveVoiceMenu = 0;
}