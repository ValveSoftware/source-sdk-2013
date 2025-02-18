//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudStalemate : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudStalemate, EditablePanel );

public:
	CHudStalemate( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent * event );
	void			SetupStalematePanel( int iReason );

private:
	Label			*m_pStalemateLabel;
	Label			*m_pReasonLabel;
	float			m_flHideAt;
};

DECLARE_HUDELEMENT( CHudStalemate );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudStalemate::CHudStalemate( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudStalemate" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flHideAt = 0;
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStalemate::Init( void )
{
	// listen for events
	ListenForGameEvent( "teamplay_round_stalemate" );

	SetVisible( false );
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStalemate::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_round_stalemate", pEventName ) == 0 )
	{
		int iReason = event->GetInt( "reason" );
		SetupStalematePanel( iReason );
		m_flHideAt = gpGlobals->curtime + 15.0;
		SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudStalemate::OnTick( void )
{
	if ( m_flHideAt && m_flHideAt < gpGlobals->curtime )
	{
		SetVisible( false );
		m_flHideAt = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStalemate::LevelInit( void )
{
	m_flHideAt = 0;
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudStalemate::ShouldDraw( void )
{
	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStalemate::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudStalemate.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pStalemateLabel = dynamic_cast<Label *>( FindChildByName("StalemateLabel") );
	m_pReasonLabel = dynamic_cast<Label *>( FindChildByName("ReasonLabel") );
}

const char *pszStalemateReasons[NUM_STALEMATE_REASONS] =
{
	"#TF_suddendeath_join",			// STALEMATE_JOIN_MID = 0,
	"#TF_suddendeath_timer",		// STALEMATE_TIMER,
	"#TF_suddendeath_limit",		// STALEMATE_SERVER_TIMELIMIT,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStalemate::SetupStalematePanel( int iReason )
{
	if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true )
	{
		if ( m_pStalemateLabel )
		{
			m_pStalemateLabel->SetText( g_pVGuiLocalize->Find( "#TF_Arena_SuddenDeathPanel" ) );
		}

		if ( m_pReasonLabel && iReason == STALEMATE_JOIN_MID )
		{
			m_pReasonLabel->SetText( g_pVGuiLocalize->Find( "#TF_Arena_SuddenDeathPanelReason" ) );
		}
	}
	else
	{
		if ( m_pStalemateLabel )
		{
			if ( iReason == STALEMATE_JOIN_MID )
			{
				m_pStalemateLabel->SetText( g_pVGuiLocalize->Find( "#TF_suddendeath_mode" ) );
			}
			else
			{
				m_pStalemateLabel->SetText( g_pVGuiLocalize->Find( "#TF_suddendeath" ) );
			}
		}

		if ( m_pReasonLabel && iReason >= 0 && iReason < NUM_STALEMATE_REASONS )
		{
			m_pReasonLabel->SetText( g_pVGuiLocalize->Find( pszStalemateReasons[iReason] ) );
		}
	}
}