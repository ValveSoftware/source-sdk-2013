//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "NavProgress.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <filesystem.h>
#include <KeyValues.h>
#include <convar.h>

#include <vgui_controls/Label.h>

#include <game/client/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//--------------------------------------------------------------------------------------------------------------
CNavProgress::CNavProgress( IViewPort *pViewPort ) : Frame( NULL, PANEL_NAV_PROGRESS )
{
	// initialize dialog
	m_pViewPort = pViewPort;

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTitle = new Label( this, "TitleLabel", "" );
	m_pText = new Label( this, "TextLabel", "" );

	m_pProgressBarBorder = new Panel( this, "ProgressBarBorder" );
	m_pProgressBar = new Panel( this, "ProgressBar" );
	m_pProgressBarSizer = new Panel( this, "ProgressBarSizer" );

	LoadControlSettings("Resource/UI/NavProgress.res");

	Reset();
}

//--------------------------------------------------------------------------------------------------------------
CNavProgress::~CNavProgress()
{
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundType( 2 );

	m_pProgressBarSizer->SetVisible( false );

	m_pProgressBarBorder->SetBorder( pScheme->GetBorder( "ButtonDepressedBorder" ) );
	m_pProgressBarBorder->SetBgColor( Color( 0, 0, 0, 0 ) );

	m_pProgressBar->SetBorder( pScheme->GetBorder( "ButtonBorder" ) );
	m_pProgressBar->SetBgColor( pScheme->GetColor( "ProgressBar.FgColor", Color( 0, 0, 0, 0 ) ) );
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_numTicks )
	{
		int w = m_pProgressBarSizer->GetWide();
		w = w * m_currentTick / m_numTicks;
		m_pProgressBar->SetWide( w );
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::Init( const char *title, int numTicks, int startTick )
{
	m_pText->SetText( title );

	m_numTicks = MAX( 1, numTicks ); // non-zero, since we'll divide by this
	m_currentTick = MAX( 0, MIN( m_numTicks, startTick ) );

	InvalidateLayout();
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::SetData(KeyValues *data)
{
	Init( data->GetString( "msg" ),
		data->GetInt( "total" ),
		data->GetInt( "current" ) );
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	m_pViewPort->ShowBackGround( bShow );

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::Reset( void )
{
}

//--------------------------------------------------------------------------------------------------------------
void CNavProgress::Update( void )
{
}

//--------------------------------------------------------------------------------------------------------------
