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
#include "tf_hud_freezepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTeamSwitch : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTeamSwitch, EditablePanel );

public:
	CHudTeamSwitch( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent * event );
	void			SetupSwitchPanel( int iNewTeam );

private:
	Label			*m_pBalanceLabel;
	float			m_flHideAt;
};

DECLARE_HUDELEMENT( CHudTeamSwitch );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTeamSwitch::CHudTeamSwitch( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTeamSwitch" )
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
void CHudTeamSwitch::Init( void )
{
	// listen for events
	ListenForGameEvent( "teamplay_teambalanced_player" );

	SetVisible( false );
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamSwitch::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_teambalanced_player", pEventName ) == 0 )
	{
		int iPlayer = event->GetInt( "player" );
		int iNewTeam = event->GetInt( "team" );
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			SetupSwitchPanel( iNewTeam );
			m_flHideAt = gpGlobals->curtime + 10.0;
			SetVisible( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudTeamSwitch::OnTick( void )
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
void CHudTeamSwitch::LevelInit( void )
{
	m_flHideAt = 0;
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTeamSwitch::ShouldDraw( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamSwitch::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudTeamSwitch.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pBalanceLabel = dynamic_cast<Label *>( FindChildByName("BalanceLabel") );
}

extern const char *pszTeamRoleSwitch[NUM_TEAM_ROLES];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamSwitch::SetupSwitchPanel( int iNewTeam )
{
	if ( m_pBalanceLabel )
	{
		C_TFTeam *pNewTeam = GetGlobalTFTeam( iNewTeam );
		if ( pNewTeam )
		{
			int iRole = pNewTeam->GetRole();
			if ( iRole > 0 && iRole < NUM_TEAM_ROLES )
			{
				m_pBalanceLabel->SetText( g_pVGuiLocalize->Find( pszTeamRoleSwitch[iRole] ) );
			}
			else if ( iRole == 0 )
			{
				if ( iNewTeam == TF_TEAM_RED )
				{
					m_pBalanceLabel->SetText( g_pVGuiLocalize->Find( "#TF_teamswitch_red" ) );
				}
				else
				{
					m_pBalanceLabel->SetText( g_pVGuiLocalize->Find( "#TF_teamswitch_blue" ) );
				}
			}
		}
	}
}