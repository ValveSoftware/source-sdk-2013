//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "vgui_rootpanel_tf.h"
#include "vgui/IVGui.h"
#include "tier2/fileutils.h"
#include "icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_TFRootPanel *g_pRootPanel = NULL;

static ConVar tf_ui_version( "tf_ui_version", "3", FCVAR_DEVELOPMENTONLY );

extern const char *COM_GetModDirectory();

void CheckCustomModSearchPaths()
{
	const char *pszCustomPathID = "custom_mod";

	CUtlVector< CUtlString > searchPaths;
	GetSearchPath( searchPaths, pszCustomPathID );

	FOR_EACH_VEC( searchPaths, i )
	{
		const char *pszSearchPath = searchPaths[i].String();
		// check each path for version file
		char szVersionFile[MAX_PATH];
		V_ComposeFileName( pszSearchPath, "info.vdf", szVersionFile, sizeof( szVersionFile ) );
		KeyValuesAD versionKV( pszCustomPathID );
		if ( versionKV->LoadFromFile( g_pFullFileSystem, szVersionFile ) )
		{
			// mod must declare this ConVar
			if ( tf_ui_version.GetInt() == versionKV->GetInt( "ui_version" ) )
			{
				continue;
			}

			DevMsg( "'ui_version' mismatch. expected version %d. Removed search path '%s' from all pathIDs.\n", tf_ui_version.GetInt(), pszSearchPath );
			// remove from all path ids
			g_pFullFileSystem->RemoveSearchPath( pszSearchPath, pszCustomPathID );
			g_pFullFileSystem->RemoveSearchPath( pszSearchPath, "game" );
			g_pFullFileSystem->RemoveSearchPath( pszSearchPath, "mod" );
		}
		else
		{
			DevMsg( "missing 'info.vdf'. Removed search path '%s' from '%s' pathID.\n", pszSearchPath, pszCustomPathID );
			g_pFullFileSystem->RemoveSearchPath( pszSearchPath, pszCustomPathID );
		}
	}

	// only allow to load loose files when using insecure mode
	if ( CommandLine()->FindParm( "-insecure" ) )
	{
		// allow lose files in these search paths
		g_pFullFileSystem->AddSearchPath( "tf", "vgui" );
		g_pFullFileSystem->AddSearchPath( "hl2", "vgui" );
		g_pFullFileSystem->AddSearchPath( "platform", "vgui" );
	}
}


//-----------------------------------------------------------------------------
// Global functions.
//-----------------------------------------------------------------------------
void VGUI_CreateClientDLLRootPanel( void )
{
	// do this before creating any vgui panels
	CheckCustomModSearchPaths();

	g_pRootPanel = new C_TFRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ) );
}

void VGUI_DestroyClientDLLRootPanel( void )
{
	g_pRootPanel->MarkForDeletion();
	g_pRootPanel = NULL;
}

vgui::VPANEL VGui_GetClientDLLRootPanel( void )
{
	return g_pRootPanel->GetVPanel();
}


//-----------------------------------------------------------------------------
// C_TFRootPanel implementation.
//-----------------------------------------------------------------------------
C_TFRootPanel::C_TFRootPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "TF Root Panel" )
{
	SetParent( parent );
	SetPaintEnabled( false );
	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	// This panel does post child painting
	SetPostChildPaintEnabled( true );

	// Make it screen sized
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	// Ask for OnTick messages
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFRootPanel::~C_TFRootPanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFRootPanel::PostChildPaint()
{
	BaseClass::PostChildPaint();

	// Draw all panel effects
	RenderPanelEffects();
}

//-----------------------------------------------------------------------------
// Purpose: For each panel effect, check if it wants to draw and draw it on
//  this panel/surface if so
//-----------------------------------------------------------------------------
void C_TFRootPanel::RenderPanelEffects( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFRootPanel::OnTick( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Reset effects on level load/shutdown
//-----------------------------------------------------------------------------
void C_TFRootPanel::LevelInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFRootPanel::LevelShutdown( void )
{
}

