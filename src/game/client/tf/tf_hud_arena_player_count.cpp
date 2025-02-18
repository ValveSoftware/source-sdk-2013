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
#include "c_tf_playerresource.h"
#include "tf_hud_arena_player_count.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


DECLARE_HUDELEMENT( CHudArenaPlayerCount );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudArenaPlayerCount::CHudArenaPlayerCount( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaPlayerCount" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::SETUP_PANEL( this );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaPlayerCount::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudArenaPlayerCount.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pBlueTeam = dynamic_cast<EditablePanel *>( FindChildByName( "blueteam" ) );
	if ( m_pBlueTeam )
	{
		m_pBlueTeam->SetDialogVariable( "blue_alive", "0" );
	}

	m_pRedTeam = dynamic_cast<EditablePanel *>( FindChildByName( "redteam" ) );
	if ( m_pRedTeam )
	{
		m_pRedTeam->SetDialogVariable( "red_alive", "0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaPlayerCount::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	if ( TFGameRules() && TFGameRules()->IsInArenaMode() && TFGameRules()->IsInWaitingForPlayers() == false )
	{
		if ( TFGameRules()->State_Get() != GR_STATE_PREROUND )
		{
			return CHudElement::ShouldDraw();
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaPlayerCount::UpdateCounts( void )
{
	// update the team sections in the scoreboard
	for ( int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_BLUE; teamIndex++ )
	{
		EditablePanel *pPanel = NULL;

		// choose dialog variables to set depending on team
		const char *pDialogVar = NULL;
		switch ( teamIndex ) 
		{
			case TF_TEAM_RED:
				pDialogVar = "red_alive";
				pPanel = m_pRedTeam;
				break;
			case TF_TEAM_BLUE:
				pDialogVar = "blue_alive";
				pPanel = m_pBlueTeam;
				break;
			default:
				Assert( false );
				break;
		}

		if ( pPanel )
		{
			// update # of living players on each team
			wchar_t wNumPlayers[6];
			int nCount = g_TF_PR ? g_TF_PR->GetNumPlayersForTeam( teamIndex, true ) : 0;

			_snwprintf( wNumPlayers, ARRAYSIZE( wNumPlayers ), L"%i", nCount );

			pPanel->SetDialogVariable( pDialogVar, wNumPlayers );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaPlayerCount::OnTick( void )
{
	if ( !ShouldDraw() )
		return;

	UpdateCounts();
}
