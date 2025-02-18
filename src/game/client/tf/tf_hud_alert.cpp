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
#include "tf_hud_freezepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudAlert : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudAlert, EditablePanel );

public:
	CHudAlert( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent * event );
	void			SetupAlertPanel( int iAlertType );

private:
	Label			*m_pAlertLabel;
	float			m_flHideAt;
};

DECLARE_HUDELEMENT( CHudAlert );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudAlert::CHudAlert( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAlert" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_flHideAt = 0;
	vgui::ivgui()->AddTickSignal( GetVPanel(), 200 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAlert::Init( void )
{
	ListenForGameEvent( "teamplay_alert" );

	SetVisible( false );
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAlert::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();
	if ( Q_strcmp( "teamplay_alert", pEventName ) == 0 )
	{
		int iAlertType = event->GetInt( "alert_type" );

		SetupAlertPanel( iAlertType );
		SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudAlert::OnTick( void )
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
void CHudAlert::LevelInit( void )
{
	m_flHideAt = 0;
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudAlert::ShouldDraw( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	if ( engine->IsPlayingDemo() )
		return false;

	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAlert::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudAlert.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pAlertLabel = dynamic_cast<Label *>( FindChildByName("AlertLabel") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAlert::SetupAlertPanel( int iAlertType )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( !m_pAlertLabel )
		return;

	// Alert case handling
	const char *pszResult = NULL;

	switch ( iAlertType )
	{
		case HUD_ALERT_SCRAMBLE_TEAMS:
			pszResult = "#game_scramble_onrestart";
			m_flHideAt = gpGlobals->curtime + 5.0;
			break;
		default:
			m_flHideAt = gpGlobals->curtime + 10.0;
			break;
	}

	if ( pszResult )
	{
		m_pAlertLabel->SetText( g_pVGuiLocalize->Find( pszResult ) );
	}
}