//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <game/client/iviewport.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <filesystem.h>
#include "IGameUIFuncs.h" // for key bindings
#include "inputsystem/iinputsystem.h"

#include "ixboxsystem.h"
#include "tf_gamerules.h"
#include "tf_controls.h"
#include "tf_shareddefs.h"
#include "tf_mapinfomenu.h"

#include "video/ivideoservices.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::CTFMapInfoMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_MAPINFO )
{
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( true );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );

	m_pTitle = new CExLabel( this, "MapInfoTitle", " " );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pContinue = new CExButton( this, "MapInfoContinue", "#TF_Continue" );
	m_pBack = new CExButton( this, "MapInfoBack", "#TF_Back" );
	m_pIntro = new CExButton( this, "MapInfoWatchIntro", "#TF_WatchIntro" );
#endif

	// info window about this map
	m_pMapInfo = new CExRichText( this, "MapInfoText" );
	m_pMapImage = new ImagePanel( this, "MapImage" );

	m_szMapName[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMapInfoMenu::~CTFMapInfoMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/MapInfoMenu_SC.res" );
		m_pContinueHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "MapInfoContinueHintIcon" ) );
		m_pBackHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "MapInfoBackHintIcon" ) );
		m_pIntroHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "MapInfoIntroHintIcon" ) );

		SetMouseInputEnabled( false );
	}
	else
	{
		LoadControlSettings( "Resource/UI/MapInfoMenu.res" );
		m_pContinueHintIcon = m_pBackHintIcon = m_pIntroHintIcon = nullptr;
		SetMouseInputEnabled( true );
	}

	CheckIntroState();
	CheckBackContinueButtons();

	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof(mapname) );

	// Save off the map name so we can re-load the page in ApplySchemeSettings().
	Q_strncpy( m_szMapName, mapname, sizeof( m_szMapName ) );
	Q_strupr( m_szMapName );

#ifdef _X360
	char *pExt = Q_stristr( m_szMapName, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	LoadMapPage();
	SetMapTitle();

#ifndef _X360
	if ( m_pContinue )
	{
		m_pContinue->RequestFocus();
	}
#endif

	SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( GetMapType( m_szMapName ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::ShowPanel( bool bShow )
{
	if ( IsVisible() == bShow )
		return;

	m_KeyRepeat.Reset();

	if ( bShow )
	{
		InvalidateLayout( true, true );		// Force scheme reload since the steam controller state may have changed.
		Activate();
		CheckIntroState();
	}
	else
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapInfoMenu::CheckForIntroMovie()
{
	const char *pVideoFileName = TFGameRules()->GetVideoFileForMap();
	if ( pVideoFileName == NULL )
	{
		return false;
	}

	VideoSystem_t  playbackSystem = VideoSystem::NONE;
	char resolvedFile[MAX_PATH];
	if ( g_pVideo && g_pVideo->LocatePlayableVideoFile( pVideoFileName, "GAME", &playbackSystem, resolvedFile, sizeof(resolvedFile) ) == VideoResult::SUCCESS  )
	{
		return true;	
	}

	return false;
}

const char *COM_GetModDirectory();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapInfoMenu::HasViewedMovieForMap()
{
	return ( UTIL_GetMapKeyCount( "viewed" ) > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckIntroState()
{
	if ( CheckForIntroMovie() && HasViewedMovieForMap() )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "intro", true );
		}
#else
		if ( m_pIntro && !m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( true );
			if ( m_pIntroHintIcon )
			{
				m_pIntroHintIcon->SetVisible( true );
			}
		}
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "intro", false );
		}
#else
		if ( m_pIntro && m_pIntro->IsVisible() )
		{
			m_pIntro->SetVisible( false );
			if ( m_pIntroHintIcon )
			{
				m_pIntroHintIcon->SetVisible( false );
			}
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::CheckBackContinueButtons()
{
#ifndef _X360
	if ( m_pBack && m_pContinue )
	{
		if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
		{
			m_pBack->SetVisible( true );
			if ( m_pBackHintIcon )
			{
				m_pBackHintIcon->SetVisible( true );
			}
			m_pContinue->SetText( "#TF_Continue" );
		}
		else
		{
			m_pBack->SetVisible( false );
			if ( m_pBackHintIcon )
			{
				m_pBackHintIcon->SetVisible( false );
			}
			m_pContinue->SetText( "#TF_Close" );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnCommand( const char *command )
{
	m_KeyRepeat.Reset();

	if ( !Q_strcmp( command, "back" ) )
	{
		 // only want to go back to the Welcome menu if we're not already on a team
		if ( !IsX360() && ( GetLocalPlayerTeam() == TEAM_UNASSIGNED ) )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_INFO, true );
		}
	}
	else if ( !Q_strcmp( command, "continue" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() && !HasViewedMovieForMap() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );

			UTIL_IncrementMapKey( "viewed" );
		}
		else
		{
			// On console, we may already have a team due to the lobby assigning us one.
			// We tell the server we're done with the map info menu, and it decides what to do with us.
			if ( IsX360() )
			{
				engine->ClientCmd( "closedwelcomemenu" );
			}
			else if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
			{
				if ( TFGameRules()->IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
				{
					m_pViewPort->ShowPanel( PANEL_ARENA_TEAM, true );
				}
				else
				{
					engine->ClientCmd( "team_ui_setup" );
				}
			}

			UTIL_IncrementMapKey( "viewed" );
		}
	}
	else if ( !Q_strcmp( command, "intro" ) )
	{
		m_pViewPort->ShowPanel( this, false );

		if ( CheckForIntroMovie() )
		{
			m_pViewPort->ShowPanel( PANEL_INTRO, true );
		}
		else
		{
			if ( TFGameRules()->IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
			{
				m_pViewPort->ShowPanel( PANEL_ARENA_TEAM, true );
			}
			else
			{
				engine->ClientCmd( "team_ui_setup" );
			}
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::Update()
{ 
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::LoadMapPage()
{
	if ( !m_szMapName[0] )
	{
		m_pMapInfo->SetText( "" );
		m_pMapImage->SetVisible( false );
		return;
	}

	// load the map image (if it exists for the current map)
	char szMapImage[ MAX_PATH ];
	Q_snprintf( szMapImage, sizeof( szMapImage ), "VGUI/maps/menu_photos_%s", m_szMapName );
	Q_strlower( szMapImage );

	IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
	if ( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
	{
		if ( m_pMapImage )
		{
			if ( !m_pMapImage->IsVisible() )
			{
				m_pMapImage->SetVisible( true );
			}

			// take off the vgui/ at the beginning when we set the image
			Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_photos_%s", m_szMapName );
			Q_strlower( szMapImage );
			
			m_pMapImage->SetImage( szMapImage );
		}
	}
	else
	{
		if ( m_pMapImage && m_pMapImage->IsVisible() )
		{
			m_pMapImage->SetVisible( false );
		}
	}

	// try loading map descriptions from the localization files first
	char mapDescriptionKey[ 64 ];
	Q_snprintf( mapDescriptionKey, sizeof( mapDescriptionKey ), "#%s_description", m_szMapName );
	Q_strlower( mapDescriptionKey );
	wchar_t* wszMapDescription = g_pVGuiLocalize->Find( mapDescriptionKey );
	if( wszMapDescription )
	{
		m_pMapInfo->SetText( wszMapDescription );
	}
	else
	{
		// try loading map descriptions from .txt files first
		char mapRES[ MAX_PATH ];

		char uilanguage[ 64 ];
		uilanguage[0] = 0;
		engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

		Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s_%s.txt", m_szMapName, uilanguage );

		// try English if the file doesn't exist for our language
		if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
		{
			Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s_english.txt", m_szMapName );

			// if the file doesn't exist for English either, try the filename without any language extension
			if( !g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
			{
				Q_snprintf( mapRES, sizeof( mapRES ), "maps/%s.txt", m_szMapName );
			}
		}

		// if no map specific description exists, load default text
		if( g_pFullFileSystem->FileExists( mapRES, "GAME" ) )
		{
			FileHandle_t f = g_pFullFileSystem->Open( mapRES, "rb" );

			// read into a memory block
			int fileSize = g_pFullFileSystem->Size(f);
			int dataSize = fileSize + sizeof( wchar_t );
			if ( dataSize % 2 )
				++dataSize;
			wchar_t *memBlock = (wchar_t *)malloc(dataSize);
			memset( memBlock, 0x0, dataSize);
			int bytesRead = g_pFullFileSystem->Read(memBlock, fileSize, f);
			if ( bytesRead < fileSize )
			{
				// NULL-terminate based on the length read in, since Read() can transform \r\n to \n and
				// return fewer bytes than we were expecting.
				char *data = reinterpret_cast<char *>( memBlock );
				data[ bytesRead ] = 0;
				data[ bytesRead+1 ] = 0;
			}

	#ifndef WIN32
			if ( ((ucs2 *)memBlock)[0] == 0xFEFF )
			{
				// convert the win32 ucs2 data to wchar_t
				dataSize*=2;// need to *2 to account for ucs2 to wchar_t (4byte) growth
				wchar_t *memBlockConverted = (wchar_t *)malloc(dataSize);	
				V_UCS2ToUnicode( (ucs2 *)memBlock, memBlockConverted, dataSize );
				free(memBlock);
				memBlock = memBlockConverted;
			}
	#else
			// null-terminate the stream (redundant, since we memset & then trimmed the transformed buffer already)
			memBlock[dataSize / sizeof(wchar_t) - 1] = 0x0000;
	#endif
			// check the first character, make sure this a little-endian unicode file

	#if defined( _X360 )
			if ( memBlock[0] != 0xFFFE )
	#else
			if ( memBlock[0] != 0xFEFF )
	#endif
			{
				// its a ascii char file
				m_pMapInfo->SetText( reinterpret_cast<char *>( memBlock ) );
			}
			else
			{
				// ensure little-endian unicode reads correctly on all platforms
				CByteswap byteSwap;
				byteSwap.SetTargetBigEndian( false );
				byteSwap.SwapBufferToTargetEndian( memBlock, memBlock, dataSize/sizeof(wchar_t) );

				m_pMapInfo->SetText( memBlock+1 );
			}
			// go back to the top of the text buffer
			m_pMapInfo->GotoTextStart();

			g_pFullFileSystem->Close( f );
			free(memBlock);
		}
		else
		{
			// try loading map descriptions from localization files next
			const char *pszDescription = NULL;
			char mapInfoKey[ 64 ];

			if ( TFGameRules() && TFGameRules()->IsPowerupMode() && ( FStrEq( m_szMapName, "ctf_foundry" ) || FStrEq( m_szMapName, "ctf_gorge" ) ) )
			{
				Q_snprintf( mapInfoKey, sizeof( mapInfoKey ), "#%s_beta", m_szMapName );
			}
			else
			{
				Q_snprintf( mapInfoKey, sizeof( mapInfoKey ), "#%s", m_szMapName );
			}
		
			Q_strlower( mapInfoKey );

			if( !g_pVGuiLocalize->Find( mapInfoKey ) )
			{
				if ( MapHasPrefix( m_szMapName, "vsh_" ) )
				{
					pszDescription = "#default_vsh_description";
				}
				else if ( MapHasPrefix( m_szMapName, "zi_" ) )
				{
					pszDescription = "#default_zi_description";
				}
				else if ( TFGameRules() )
				{
					if ( TFGameRules()->IsMannVsMachineMode() )
					{
						pszDescription = "#default_mvm_description";
					}
					else
					{
						switch ( TFGameRules()->GetGameType() )
						{
						case TF_GAMETYPE_CTF:
							pszDescription = "#default_ctf_description";
							break;
						case TF_GAMETYPE_CP:
							if ( TFGameRules()->IsInKothMode() )
							{
								pszDescription = "#default_koth_description";
							}
							else
							{
								pszDescription = "#default_cp_description";
							}
							break;
						case TF_GAMETYPE_ESCORT:
							if ( TFGameRules()->HasMultipleTrains() )
							{
								pszDescription = "#default_payload_race_description";
							}
							else
							{
								pszDescription = "#default_payload_description";
							}
							break;
						case TF_GAMETYPE_ARENA:
							pszDescription = "#default_arena_description";
							break;
						case TF_GAMETYPE_RD:
							pszDescription = "#default_rd_description";
							break;
						case TF_GAMETYPE_PASSTIME:
							pszDescription = "#default_passtime_description";
							break;
						case TF_GAMETYPE_PD:
							pszDescription = "#default_pd_description";
							break;
						}
					}
				}
			}
			else
			{
				pszDescription = mapInfoKey;
			}

			if ( pszDescription && pszDescription[0] )
			{
				m_pMapInfo->SetText( pszDescription );
			}
			else
			{
				m_pMapInfo->SetText( "" );
			}
		}
	}

	// we haven't loaded a valid map image for the current map
	if ( m_pMapImage && !m_pMapImage->IsVisible() )
	{
		if ( m_pMapInfo )
		{
			m_pMapInfo->SetWide( m_pMapInfo->GetWide() + ( m_pMapImage->GetWide() * 0.75 ) ); // add in the extra space the images would have taken 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::SetMapTitle()
{
	SetDialogVariable( "mapname", GetMapDisplayName( m_szMapName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( code == KEY_XBUTTON_A || code == STEAMCONTROLLER_A )
	{
		OnCommand( "continue" );
	}
	else if ( code == STEAMCONTROLLER_B )
	{
		OnCommand( "back" );
	}
	else if ( code == KEY_XBUTTON_Y || code == STEAMCONTROLLER_Y )
	{
		OnCommand( "intro" );
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP || code == STEAMCONTROLLER_DPAD_UP )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", 1) );
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN || code == STEAMCONTROLLER_DPAD_DOWN )
	{
		// Scroll class info text up
		if ( m_pMapInfo )
		{
			PostMessage( m_pMapInfo, new KeyValues("MoveScrollBarDirect", "delta", -1) );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapInfoMenu::OnThink()
{
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	//Always hide the health... this needs to be done every frame because a message from the server keeps resetting this.
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}

