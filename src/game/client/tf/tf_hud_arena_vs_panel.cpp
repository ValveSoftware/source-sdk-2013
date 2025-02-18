//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tf_gamerules.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_hud_arena_vs_panel.h"
#include "vgui_avatarimage.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


DECLARE_HUDELEMENT( CHudArenaVsPanel );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudArenaVsPanel::CHudArenaVsPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaVsPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::SETUP_PANEL( this );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_bVisible = false;
	m_flHideTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaVsPanel::Init( void )
{
	CHudElement::Init();

	ListenForGameEvent( "show_vs_panel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaVsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudArenaVsPanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pBluePanel = dynamic_cast<EditablePanel *>( FindChildByName("bluepanel") );
	if ( m_pBluePanel )
	{
		m_pBlueAvatar = dynamic_cast<CAvatarImagePanel *>( m_pBluePanel->FindChildByName("AvatarImage") );
	}

	m_pRedPanel = dynamic_cast<EditablePanel *>( FindChildByName("redpanel") );
	if ( m_pRedPanel )
	{
		m_pRedAvatar = dynamic_cast<CAvatarImagePanel *>( m_pRedPanel->FindChildByName("AvatarImage") );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaVsPanel::ShouldDraw( void )
{
	if ( TFGameRules() && TFGameRules()->IsInArenaMode() )
	{
		return m_bVisible;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaVsPanel::OnTick( void )
{
	if ( !m_bVisible )
		return;

	// hide us after some time just incase
	if ( gpGlobals->curtime > m_flHideTime )
	{
		m_bVisible = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaVsPanel::FireGameEvent( IGameEvent * event )
{
	const char *eventName = event->GetName();

	if ( FStrEq( "show_vs_panel", eventName ) )
	{
		m_bVisible = event->GetBool( "show", false );

		if ( m_bVisible )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ArenaVsPanelOnShow" );			

			m_flHideTime = gpGlobals->curtime + 10.0f;

			// grab the data to display
			// player index of each of the team leaders

			// set up timers for animation events

			// grab the leader of each team

			C_TFTeam *pBlue = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pBlue )
			{
				C_BasePlayer *pBlueLeader = pBlue->GetTeamLeader();
				if ( pBlueLeader )
				{
					char buf[100];
					Q_snprintf( buf, sizeof(buf), "Team %s", pBlueLeader->GetPlayerName() );
					m_pBluePanel->SetDialogVariable( "blueleader", buf );

				}
				else
				{
					m_pBluePanel->SetDialogVariable( "blueleader", "Team BLU" );
				}

				m_pBlueAvatar->SetPlayer( pBlueLeader );
			}

			C_TFTeam *pRed = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pRed )
			{
				C_BasePlayer *pRedLeader = pRed->GetTeamLeader();
				if ( pRedLeader )
				{
					char buf[100];
					Q_snprintf( buf, sizeof(buf), "Team %s", pRedLeader->GetPlayerName() );
					m_pRedPanel->SetDialogVariable( "redleader", buf );
				}
				else
				{
					m_pRedPanel->SetDialogVariable( "redleader", "Team RED" );
				}

				m_pRedAvatar->SetPlayer( pRedLeader );
			}
		}
	}	
}