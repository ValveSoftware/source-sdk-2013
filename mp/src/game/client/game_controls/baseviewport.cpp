//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client DLL VGUI2 Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#pragma warning( disable : 4800  )  // disable forcing int to bool performance warning

#include "cbase.h"
#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>

// VGUI panel includes
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui_controls/Button.h>

#include <igameresources.h>

// sub dialogs
#include "clientscoreboarddialog.h"
#include "spectatorgui.h"
#include "teammenu.h"
#include "vguitextwindow.h"
#include "IGameUIFuncs.h"
#include "mapoverview.h"
#include "hud.h"
#include "NavProgress.h"
#include "commentary_modelviewer.h"

// our definition
#include "baseviewport.h"
#include <filesystem.h>
#include <convar.h>
#include "ienginevgui.h"
#include "iclientmode.h"

#include "tier0/etwprof.h"

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IViewPort *gViewPortInterface = NULL;

vgui::Panel *g_lastPanel = NULL; // used for mouseover buttons, keeps track of the last active panel
vgui::Button *g_lastButton = NULL; // used for mouseover buttons, keeps track of the last active button
using namespace vgui;

void hud_autoreloadscript_callback( IConVar *var, const char *pOldValue, float flOldValue );

ConVar hud_autoreloadscript("hud_autoreloadscript", "0", FCVAR_NONE, "Automatically reloads the animation script each time one is ran", hud_autoreloadscript_callback);

void hud_autoreloadscript_callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	if ( g_pClientMode && g_pClientMode->GetViewportAnimationController() )
	{
		g_pClientMode->GetViewportAnimationController()->SetAutoReloadScript( hud_autoreloadscript.GetBool() );
	}
}

static ConVar cl_leveloverviewmarker( "cl_leveloverviewmarker", "0", FCVAR_CHEAT );

CON_COMMAND( showpanel, "Shows a viewport panel <name>" )
{
	if ( !gViewPortInterface )
		return;
	
	if ( args.ArgC() != 2 )
		return;
		
	 gViewPortInterface->ShowPanel( args[ 1 ], true );
}

CON_COMMAND( hidepanel, "Hides a viewport panel <name>" )
{
	if ( !gViewPortInterface )
		return;
	
	if ( args.ArgC() != 2 )
		return;
		
	 gViewPortInterface->ShowPanel( args[ 1 ], false );
}

/* global helper functions

bool Helper_LoadFile( IBaseFileSystem *pFileSystem, const char *pFilename, CUtlVector<char> &buf )
{
	FileHandle_t hFile = pFileSystem->Open( pFilename, "rt" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		Warning( "Helper_LoadFile: missing %s\n", pFilename );
		return false;
	}

	unsigned long len = pFileSystem->Size( hFile );
	buf.SetSize( len );
	pFileSystem->Read( buf.Base(), buf.Count(), hFile );
	pFileSystem->Close( hFile );

	return true;
} */

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseViewport::LoadHudAnimations( void )
{
	const char *HUDANIMATION_MANIFEST_FILE = "scripts/hudanimations_manifest.txt";
	KeyValues *manifest = new KeyValues( HUDANIMATION_MANIFEST_FILE );
	if ( manifest->LoadFromFile( g_pFullFileSystem, HUDANIMATION_MANIFEST_FILE, "GAME" ) == false )
	{
		manifest->deleteThis();
		return false;
	}

	bool bClearScript = true;

	// Load each file defined in the text
	for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
	{
		if ( !Q_stricmp( sub->GetName(), "file" ) )
		{
			// Add it
			if ( m_pAnimController->SetScriptFile( GetVPanel(), sub->GetString(), bClearScript ) == false )
			{
				Assert( 0 );
			}

			bClearScript = false;
			continue;
		}
	}

	manifest->deleteThis();
	return true;
}

//================================================================
CBaseViewport::CBaseViewport() : vgui::EditablePanel( NULL, "CBaseViewport")
{
	SetSize( 10, 10 ); // Quiet "parent not sized yet" spew
	gViewPortInterface = this;
	m_bInitialized = false;

	m_GameuiFuncs = NULL;
	m_GameEventManager = NULL;
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

#ifndef _XBOX
	m_pBackGround = NULL;
#endif
	m_bHasParent = false;
	m_pActivePanel = NULL;
	m_pLastActivePanel = NULL;
	g_lastPanel = NULL;

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	m_pAnimController = new vgui::AnimationController(this);
	// create our animation controller
	m_pAnimController->SetScheme(scheme);
	m_pAnimController->SetProportional(true);
	
	// Attempt to load all hud animations
	if ( LoadHudAnimations() == false )
	{
		// Fall back to just the main
		if ( m_pAnimController->SetScriptFile( GetVPanel(), "scripts/HudAnimations.txt", true ) == false )
		{
			Assert(0);
		}
	}

	m_OldSize[ 0 ] = m_OldSize[ 1 ] = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Updates hud to handle the new screen size
//-----------------------------------------------------------------------------
void CBaseViewport::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);

	IViewPortPanel* pSpecGuiPanel = FindPanelByName(PANEL_SPECGUI);
	bool bSpecGuiWasVisible = pSpecGuiPanel && pSpecGuiPanel->IsVisible();
	
	// reload the script file, so the screen positions in it are correct for the new resolution
	ReloadScheme( NULL );

	// recreate all the default panels
	RemoveAllPanels();
#ifndef _XBOX
	m_pBackGround = new CBackGroundPanel( NULL );
	m_pBackGround->SetZPos( -20 ); // send it to the back 
	m_pBackGround->SetVisible( false );
#endif
	CreateDefaultPanels();
#ifndef _XBOX
	vgui::ipanel()->MoveToBack( m_pBackGround->GetVPanel() ); // really send it to the back 
#endif

	// hide all panels when reconnecting 
	ShowPanel( PANEL_ALL, false );

	// re-enable the spectator gui if it was previously visible
	if ( bSpecGuiWasVisible )
	{
		ShowPanel( PANEL_SPECGUI, true );
	}
}

void CBaseViewport::CreateDefaultPanels( void )
{
#ifndef _XBOX
	AddNewPanel( CreatePanelByName( PANEL_SCOREBOARD ), "PANEL_SCOREBOARD" );
	AddNewPanel( CreatePanelByName( PANEL_INFO ), "PANEL_INFO" );
	AddNewPanel( CreatePanelByName( PANEL_SPECGUI ), "PANEL_SPECGUI" );
#if !defined( TF_CLIENT_DLL )
	AddNewPanel( CreatePanelByName( PANEL_SPECMENU ), "PANEL_SPECMENU" );
	AddNewPanel( CreatePanelByName( PANEL_NAV_PROGRESS ), "PANEL_NAV_PROGRESS" );
#endif // !TF_CLIENT_DLL
#endif // !_XBOX
}

void CBaseViewport::UpdateAllPanels( void )
{
	int count = m_Panels.Count();

	for (int i=0; i< count; i++ )
	{
		IViewPortPanel *p = m_Panels[i];

		if ( p->IsVisible() )
		{
			p->Update();
		}
	}
}

IViewPortPanel* CBaseViewport::CreatePanelByName(const char *szPanelName)
{
	IViewPortPanel* newpanel = NULL;

#ifndef _XBOX
	if ( Q_strcmp(PANEL_SCOREBOARD, szPanelName) == 0 )
	{
		newpanel = new CClientScoreBoardDialog( this );
	}
	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CTextWindow( this );
	}
/*	else if ( Q_strcmp(PANEL_OVERVIEW, szPanelName) == 0 )
	{
		newpanel = new CMapOverview( this );
	}
	*/
	else if ( Q_strcmp(PANEL_TEAM, szPanelName) == 0 )
	{
		newpanel = new CTeamMenu( this );
	}
	else if ( Q_strcmp(PANEL_SPECMENU, szPanelName) == 0 )
	{
		newpanel = new CSpectatorMenu( this );
	}
	else if ( Q_strcmp(PANEL_SPECGUI, szPanelName) == 0 )
	{
		newpanel = new CSpectatorGUI( this );
	}
#if !defined( TF_CLIENT_DLL )
	else if ( Q_strcmp(PANEL_NAV_PROGRESS, szPanelName) == 0 )
	{
		newpanel = new CNavProgress( this );
	}
#endif	// TF_CLIENT_DLL
#endif

	if ( Q_strcmp(PANEL_COMMENTARY_MODELVIEWER, szPanelName) == 0 )
	{
		newpanel = new CCommentaryModelViewer( this );
	}
	
	return newpanel; 
}


bool CBaseViewport::AddNewPanel( IViewPortPanel* pPanel, char const *pchDebugName )
{
	if ( !pPanel )
	{
		DevMsg("CBaseViewport::AddNewPanel(%s): NULL panel.\n", pchDebugName );
		return false;
	}

	// we created a new panel, initialize it
	if ( FindPanelByName( pPanel->GetName() ) != NULL )
	{
		DevMsg("CBaseViewport::AddNewPanel: panel with name '%s' already exists.\n", pPanel->GetName() );
		return false;
	}

	m_Panels.AddToTail( pPanel );
	pPanel->SetParent( GetVPanel() );
	
	return true;
}

IViewPortPanel* CBaseViewport::FindPanelByName(const char *szPanelName)
{
	int count = m_Panels.Count();

	for (int i=0; i< count; i++ )
	{
		if ( Q_strcmp(m_Panels[i]->GetName(), szPanelName) == 0 )
			return m_Panels[i];
	}

	return NULL;
}

void CBaseViewport::PostMessageToPanel( IViewPortPanel* pPanel, KeyValues *pKeyValues )
{			   
	PostMessage( pPanel->GetVPanel(), pKeyValues );
}

void CBaseViewport::PostMessageToPanel( const char *pName, KeyValues *pKeyValues )
{
	if ( Q_strcmp( pName, PANEL_ALL ) == 0 )
	{
		for (int i=0; i< m_Panels.Count(); i++ )
		{
			PostMessageToPanel( m_Panels[i], pKeyValues );
		}

		return;
	}

	IViewPortPanel * panel = NULL;

	if ( Q_strcmp( pName, PANEL_ACTIVE ) == 0 )
	{
		panel = m_pActivePanel;
	}
	else
	{
		panel = FindPanelByName( pName );
	}

	if ( !panel	)
		return;

	PostMessageToPanel( panel, pKeyValues );
}


void CBaseViewport::ShowPanel( const char *pName, bool state )
{
	if ( Q_strcmp( pName, PANEL_ALL ) == 0 )
	{
		for (int i=0; i< m_Panels.Count(); i++ )
		{
			ShowPanel( m_Panels[i], state );
		}

		return;
	}

	IViewPortPanel * panel = NULL;

	if ( Q_strcmp( pName, PANEL_ACTIVE ) == 0 )
	{
		panel = m_pActivePanel;
	}
	else
	{
		panel = FindPanelByName( pName );
	}
	
	if ( !panel	)
		return;

	ShowPanel( panel, state );
}

void CBaseViewport::ShowPanel( IViewPortPanel* pPanel, bool state )
{
	if ( state )
	{
		// if this is an 'active' panel, deactivate old active panel
		if ( pPanel->HasInputElements() )
		{
			// don't show input panels during normal demo playback
#if defined( REPLAY_ENABLED )
			if ( engine->IsPlayingDemo() && !engine->IsHLTV() && !g_pEngineClientReplay->IsPlayingReplayDemo() )
#else
			if ( engine->IsPlayingDemo() && !engine->IsHLTV() )
#endif
				return;
			if ( (m_pActivePanel != NULL) && (m_pActivePanel != pPanel) && (m_pActivePanel->IsVisible()) )
			{
				// store a pointer to the currently active panel
				// so we can restore it later
				m_pLastActivePanel = m_pActivePanel;
				m_pActivePanel->ShowPanel( false );
			}
		
			m_pActivePanel = pPanel;
		}
	}
	else
	{
		// if this is our current active panel
		// update m_pActivePanel pointer
		if ( m_pActivePanel == pPanel )
		{
			m_pActivePanel = NULL;
		}

		// restore the previous active panel if it exists
		if( m_pLastActivePanel )
		{
			m_pActivePanel = m_pLastActivePanel;
			m_pLastActivePanel = NULL;

			m_pActivePanel->ShowPanel( true );
		}
	}

	// just show/hide panel
	pPanel->ShowPanel( state );

	UpdateAllPanels(); // let other panels rearrange
}

IViewPortPanel* CBaseViewport::GetActivePanel( void )
{
	return m_pActivePanel;
}

void CBaseViewport::RemoveAllPanels( void)
{
	g_lastPanel = NULL;
	for ( int i=0; i < m_Panels.Count(); i++ )
	{
		vgui::VPANEL vPanel = m_Panels[i]->GetVPanel();
		vgui::ipanel()->DeletePanel( vPanel );
	}
#ifndef _XBOX
	if ( m_pBackGround )
	{
		m_pBackGround->MarkForDeletion();
		m_pBackGround = NULL;
	}
#endif
	m_Panels.Purge();
	m_pActivePanel = NULL;
	m_pLastActivePanel = NULL;
}

CBaseViewport::~CBaseViewport()
{
	m_bInitialized = false;

#ifndef _XBOX
	if ( !m_bHasParent && m_pBackGround )
	{
		m_pBackGround->MarkForDeletion();
	}
	m_pBackGround = NULL;
#endif
	RemoveAllPanels();

	gameeventmanager->RemoveListener( this );
}


//-----------------------------------------------------------------------------
// Purpose: called when the VGUI subsystem starts up
//			Creates the sub panels and initialises them
//-----------------------------------------------------------------------------
void CBaseViewport::Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager2 * pGameEventManager )
{
	m_GameuiFuncs = pGameUIFuncs;
	m_GameEventManager = pGameEventManager;
#ifndef _XBOX
	m_pBackGround = new CBackGroundPanel( NULL );
	m_pBackGround->SetZPos( -20 ); // send it to the back 
	m_pBackGround->SetVisible( false );
#endif
	CreateDefaultPanels();

	m_GameEventManager->AddListener( this, "game_newmap", false );
	
	m_bInitialized = true;
}

/*

//-----------------------------------------------------------------------------
// Purpose: Updates the spectator panel with new player info
//-----------------------------------------------------------------------------
void CBaseViewport::UpdateSpectatorPanel()
{
	char bottomText[128];
	int player = -1;
	const char *name;
	Q_snprintf(bottomText,sizeof( bottomText ), "#Spec_Mode%d", m_pClientDllInterface->SpectatorMode() );

	m_pClientDllInterface->CheckSettings();
	// check if we're locked onto a target, show the player's name
	if ( (m_pClientDllInterface->SpectatorTarget() > 0) && (m_pClientDllInterface->SpectatorTarget() <= m_pClientDllInterface->GetMaxPlayers()) && (m_pClientDllInterface->SpectatorMode() != OBS_ROAMING) )
	{
		player = m_pClientDllInterface->SpectatorTarget();
	}

		// special case in free map and inset off, don't show names
	if ( ((m_pClientDllInterface->SpectatorMode() == OBS_MAP_FREE) && !m_pClientDllInterface->PipInsetOff()) || player == -1 )
		name = NULL;
	else
		name = m_pClientDllInterface->GetPlayerInfo(player).name;

	// create player & health string
	if ( player && name )
	{
		Q_strncpy( bottomText, name, sizeof( bottomText ) );
	}
	char szMapName[64];
	Q_FileBase( const_cast<char *>(m_pClientDllInterface->GetLevelName()), szMapName );

	m_pSpectatorGUI->Update(bottomText, player, m_pClientDllInterface->SpectatorMode(), m_pClientDllInterface->IsSpectateOnly(), m_pClientDllInterface->SpectatorNumber(), szMapName );
	m_pSpectatorGUI->UpdateSpectatorPlayerList();
}  */

// Return TRUE if the HUD's allowed to print text messages
bool CBaseViewport::AllowedToPrintText( void )
{

	/* int iId = GetCurrentMenuID();
	if ( iId == MENU_TEAM || iId == MENU_CLASS || iId == MENU_INTRO || iId == MENU_CLASSHELP )
		return false; */
	// TODO ask every aktive elemet if it allows to draw text while visible

	return ( m_pActivePanel == NULL);
} 

void CBaseViewport::OnThink()
{
	// Clear our active panel pointer if the panel has made
	// itself invisible. Need this so we don't bring up dead panels
	// if they are stored as the last active panel
	if( m_pActivePanel && !m_pActivePanel->IsVisible() )
	{
		if( m_pLastActivePanel )
		{
			m_pActivePanel = m_pLastActivePanel;
			ShowPanel( m_pActivePanel, true );
			m_pLastActivePanel = NULL;
		}
		else
			m_pActivePanel = NULL;
	}

	// TF does this in OnTick in TFViewport.  This remains to preserve old
	// behavior in other games
#if !defined( TF_CLIENT_DLL )
	m_pAnimController->UpdateAnimations( gpGlobals->curtime );
#endif

	int count = m_Panels.Count();

	for (int i=0; i< count; i++ )
	{
		IViewPortPanel *panel = m_Panels[i];
		if ( panel->NeedsUpdate() && panel->IsVisible() )
		{
			panel->Update();
		}
	}

	int w, h;
	vgui::ipanel()->GetSize( enginevgui->GetPanel( PANEL_CLIENTDLL ), w, h );

	if ( m_OldSize[ 0 ] != w || m_OldSize[ 1 ] != h )
	{
		m_OldSize[ 0 ] = w;
		m_OldSize[ 1 ] = h;
		g_pClientMode->Layout();
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the parent for each panel to use
//-----------------------------------------------------------------------------
void CBaseViewport::SetParent(vgui::VPANEL parent)
{
	EditablePanel::SetParent( parent );
	// force ourselves to be proportional - when we set our parent above, if our new
	// parent happened to be non-proportional (such as the vgui root panel), we got
	// slammed to be nonproportional
	EditablePanel::SetProportional( true );
	
#ifndef _XBOX
	m_pBackGround->SetParent( (vgui::VPANEL)parent );
#endif

	// set proportionality on animation controller
	m_pAnimController->SetProportional( true );

	m_bHasParent = (parent != 0);
}

//-----------------------------------------------------------------------------
// Purpose: called when the engine shows the base client VGUI panel (i.e when entering a new level or exiting GameUI )
//-----------------------------------------------------------------------------
void CBaseViewport::ActivateClientUI() 
{
}

//-----------------------------------------------------------------------------
// Purpose: called when the engine hides the base client VGUI panel (i.e when the GameUI is comming up ) 
//-----------------------------------------------------------------------------
void CBaseViewport::HideClientUI()
{
}

//-----------------------------------------------------------------------------
// Purpose: passes death msgs to the scoreboard to display specially
//-----------------------------------------------------------------------------
void CBaseViewport::FireGameEvent( IGameEvent * event)
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		// hide all panels when reconnecting 
		ShowPanel( PANEL_ALL, false );

		if ( engine->IsHLTV() )
		{
			ShowPanel( PANEL_SPECGUI, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseViewport::ReloadScheme(const char *fromFile)
{
	CETWScope timer( "CBaseViewport::ReloadScheme" );

	// See if scheme should change
	
	if ( fromFile != NULL )
	{
		// "resource/ClientScheme.res"
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), fromFile, "HudScheme" );

		SetScheme(scheme);
		SetProportional( true );
		m_pAnimController->SetScheme(scheme);
	}

	// Force a reload
	if ( LoadHudAnimations() == false )
	{
		// Fall back to just the main
		if ( m_pAnimController->SetScriptFile( GetVPanel(), "scripts/HudAnimations.txt", true ) == false )
		{
			Assert(0);
		}
	}

	SetProportional( true );
	
	KeyValuesAD pConditions( "conditions" );
	g_pClientMode->ComputeVguiResConditions( pConditions );

	// reload the .res file from disk
	LoadControlSettings( "scripts/HudLayout.res", NULL, NULL, pConditions );

	gHUD.RefreshHudTextures();

	InvalidateLayout( true, true );

	// reset the hud
	gHUD.ResetHUD();
}

int CBaseViewport::GetDeathMessageStartHeight( void )
{
	return YRES(2);
}

void CBaseViewport::Paint()
{
	if ( cl_leveloverviewmarker.GetInt() > 0 )
	{
		int size = cl_leveloverviewmarker.GetInt();
		// draw a 1024x1024 pixel box
		vgui::surface()->DrawSetColor( 255, 0, 0, 255 );
		vgui::surface()->DrawLine( size, 0, size, size );
		vgui::surface()->DrawLine( 0, size, size, size );
	}
}
