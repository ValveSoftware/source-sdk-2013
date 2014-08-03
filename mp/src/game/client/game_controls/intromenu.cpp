//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "intromenu.h"
#include <networkstringtabledefs.h>
#include <cdll_client_int.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <filesystem.h>
#include <KeyValues.h>
#include <convar.h>
#include <game/client/iviewport.h>
#include "spectatorgui.h"
#include "gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CIntroMenu::CIntroMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_INTRO )
{
	// initialize dialog
	m_pViewPort = pViewPort;

	m_pTitleLabel = NULL;

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	SetMoveable( false );
	SetSizeable( false );
	SetProportional( true );

	// hide the system buttons
	SetTitleBarVisible( false );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CIntroMenu::~CIntroMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the color of the top and bottom bars
//-----------------------------------------------------------------------------
void CIntroMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings("Resource/UI/IntroMenu.res");

	m_pTitleLabel = dynamic_cast<Label *>( FindChildByName( "titlelabel" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CIntroMenu::Reset( void )
{
	Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CIntroMenu::Update( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CIntroMenu::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "skip" ) )
	{
		engine->ClientCmd( "intro_skip" );
		m_pViewPort->ShowPanel( this, false );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CIntroMenu::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	m_pViewPort->ShowBackGround( bShow );

	if ( bShow )
	{
		Activate();

		if ( GameRules() )
		{
			SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( GameRules()->GetGameTypeName() ) );
		}

		SetMouseInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}
