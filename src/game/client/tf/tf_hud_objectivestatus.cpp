//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_flagstatus.h"
#include "tf_hud_objectivestatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_arena_player_count.h"
#include "c_tf_playerresource.h"
#include "tf_hud_robot_destruction_status.h"
#include "tf_hud_passtime.h"
#include "c_tf_passtime_logic.h"

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

ConVar tf_hud_show_servertimelimit( "tf_hud_show_servertimelimit", "0", FCVAR_ARCHIVE, "Display time left before the current map ends." );

extern ConVar tf_arena_round_time;

using namespace vgui;

DECLARE_HUDELEMENT( CTFHudObjectiveStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudObjectiveStatus::CTFHudObjectiveStatus( const char *pElementName ) 
	: CHudElement( pElementName )
	, BaseClass( NULL, "HudObjectiveStatus" ) 
	, m_pFlagPanel( NULL )
	, m_pControlPointIconsPanel( NULL )
	, m_pControlPointProgressBar( NULL )
	, m_pEscortPanel( NULL )
	, m_pMultipleEscortPanel( NULL )
	, m_pTrainingPanel( NULL )
	, m_pRobotDestructionPanel( NULL )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pFlagPanel = new CTFHudFlagObjectives( this, "ObjectiveStatusFlagPanel" );
	m_pControlPointIconsPanel = NULL;
	m_pControlPointProgressBar = new CControlPointProgressBar( this );
	m_pEscortPanel = new CTFHudEscort( this, "ObjectiveStatusEscort" );
	m_pMultipleEscortPanel = new CTFHudMultipleEscort( this, "ObjectiveStatusMultipleEscort" );
	m_pTrainingPanel = new CTFHudTraining(this, "ObjectiveStatusTraining" );
	m_pRobotDestructionPanel = NULL;
	m_pHudPasstime = new CTFHudPasstime( this );

	SetHiddenBits( 0 );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::ApplySchemeSettings( IScheme *pScheme )
{
	if ( m_pRobotDestructionPanel )
	{
		m_pRobotDestructionPanel->MarkForDeletion();
		m_pRobotDestructionPanel = NULL;
	}
	m_pRobotDestructionPanel = new CTFHUDRobotDestruction( this, "ObjectiveStatusRobotDestruction" );

	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveStatus.res" );

	if ( !m_pControlPointIconsPanel )
	{
		m_pControlPointIconsPanel = GET_HUDELEMENT( CHudControlPointIcons );
		m_pControlPointIconsPanel->SetParent( this );
	}

	if ( m_pControlPointProgressBar )
	{
		m_pControlPointProgressBar->InvalidateLayout( true, true );
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::Reset()
{
	if ( m_pFlagPanel )
	{
		m_pFlagPanel->Reset();
	}

	if ( m_pEscortPanel )
	{
		m_pEscortPanel->Reset();
	}

	if ( m_pMultipleEscortPanel )
	{
		m_pMultipleEscortPanel->Reset();
	}

	if ( m_pControlPointProgressBar )
	{
		m_pControlPointProgressBar->Reset();
	}

	if ( m_pRobotDestructionPanel )
	{
		m_pRobotDestructionPanel->Reset();
	}

	if ( m_pHudPasstime )
	{
		m_pHudPasstime->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlPointProgressBar *CTFHudObjectiveStatus::GetControlPointProgressBar( void )
{
	return m_pControlPointProgressBar;
}

//=============================================================================
// HPE_BEGIN
// [msmith] Functions for training stuff.
//=============================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::SetTrainingText( char *text )
{
	if ( NULL == m_pTrainingPanel )
  		return;

	m_pTrainingPanel->SetTrainingText( text );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::SetTrainingObjective( char *text )
{
	if ( NULL == m_pTrainingPanel )
		return;

	m_pTrainingPanel->SetTrainingObjective( text );
}
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::SetVisiblePanels( void )
{
	if ( !TFGameRules() )
		return;

	//=============================================================================
	// HPE_BEGIN
	// [msmith] Added stuff related to showing different HUDs for training.
	//=============================================================================
	int iGameType = TFGameRules()->GetGameType();
	int iHudType = TFGameRules()->GetHUDType();

	bool bIsPlayingRobotDestruction = TFGameRules()->IsPlayingRobotDestructionMode();
	if ( m_pRobotDestructionPanel && m_pRobotDestructionPanel->IsVisible() != bIsPlayingRobotDestruction )
	{
		m_pRobotDestructionPanel->SetVisible( bIsPlayingRobotDestruction );
	}

	bool bCTFVisible = TFGameRules()->IsPlayingHybrid_CTF_CP();
	if ( !bCTFVisible )
	{
		bCTFVisible = ( iGameType == TF_GAMETYPE_CTF || iHudType == TF_HUDTYPE_CTF ) && ( iHudType != TF_HUDTYPE_CP ) && ( iHudType != TF_HUDTYPE_ESCORT );
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		bCTFVisible = ( ( TFGameRules()->State_Get() != GR_STATE_BETWEEN_RNDS ) 
					 && ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN ) 
					 && ( TFGameRules()->State_Get() != GR_STATE_GAME_OVER ) );
	}

	//First check to see if we have an override HUD for the training simulation.
	//If we do, ignore any other game play hud displays.
	if ( iHudType == TF_HUDTYPE_TRAINING )
	{
		m_pTrainingPanel->SetVisible(true);
		if ( m_pFlagPanel )
		{
			m_pFlagPanel->SetVisible( false );
		}
		
		if ( m_pControlPointIconsPanel )
		{
			m_pControlPointIconsPanel->SetVisible( false );
		}
		
		if ( m_pEscortPanel )
		{
			m_pEscortPanel->SetVisible( false );
		}
		
		if ( m_pMultipleEscortPanel )
		{
			m_pMultipleEscortPanel->SetVisible( false );
		}
		
		if ( m_pHudPasstime )
		{
			m_pHudPasstime->SetVisible( false );
			m_pHudPasstime->SetEnabled( false );
		}

		return;
	}

	m_pTrainingPanel->SetVisible( TFGameRules()->IsTrainingHUDVisible() );

	if ( m_pFlagPanel && m_pFlagPanel->IsVisible() != bCTFVisible )
	{
		m_pFlagPanel->SetVisible( bCTFVisible );
	}

	bool bCPVisible = TFGameRules()->IsPlayingHybrid_CTF_CP();
	if ( !bCPVisible )
	{
		bCPVisible = ( iGameType == TF_GAMETYPE_CP || iGameType == TF_GAMETYPE_ARENA || iHudType == TF_HUDTYPE_CP || iGameType == TF_GAMETYPE_MVM ) && ( iHudType != TF_HUDTYPE_CTF ) && ( iHudType != TF_HUDTYPE_ESCORT ) && !TFGameRules()->IsPlayingHybrid_CTF_CP();
	}

	if ( m_pControlPointIconsPanel && m_pControlPointIconsPanel->IsVisible() != bCPVisible )
	{
		m_pControlPointIconsPanel->SetVisible( bCPVisible );
	}

	bool bEscortVisible = ( iGameType == TF_GAMETYPE_ESCORT || iHudType == TF_HUDTYPE_ESCORT ) && ( iHudType != TF_HUDTYPE_CTF ) && ( iHudType != TF_HUDTYPE_CP ) && !TFGameRules()->IsPlayingHybrid_CTF_CP();
	if ( bEscortVisible )
	{
		bool bMultipleTrains = TFGameRules()->HasMultipleTrains();

		if ( m_pEscortPanel && m_pEscortPanel->EditablePanel::IsVisible() != !bMultipleTrains ) // intentionally skipping EscortPanel version of IsVisible() to bypass the !m_bHaveValidPointPositions check
		{
			m_pEscortPanel->SetVisible( !bMultipleTrains );
		}

		// Hide the panel while players are fighting in Helltower's hell
		if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) && ( TFGameRules()->ArePlayersInHell() == true ) )
		{
			bMultipleTrains = false;
		}

		if ( m_pMultipleEscortPanel && m_pMultipleEscortPanel->EditablePanel::IsVisible() != bMultipleTrains ) // intentionally skipping EscortPanel version of IsVisible() to bypass the !m_bHaveValidPointPositions check
		{
			m_pMultipleEscortPanel->SetVisible( bMultipleTrains );
		}
	}
	else
	{
		if ( m_pEscortPanel && m_pEscortPanel->EditablePanel::IsVisible() ) // intentionally skipping EscortPanel version of IsVisible() to bypass the !m_bHaveValidPointPositions check
		{
			m_pEscortPanel->SetVisible( false );
		}

		if ( m_pMultipleEscortPanel && m_pMultipleEscortPanel->EditablePanel::IsVisible() ) // intentionally skipping EscortPanel version of IsVisible() to bypass the !m_bHaveValidPointPositions check
		{
			m_pMultipleEscortPanel->SetVisible( false );
		}
	}

	if ( m_pHudPasstime )
	{
		bool bIsPasstime = iGameType == TF_GAMETYPE_PASSTIME;
		m_pHudPasstime->SetVisible( bIsPasstime );
		m_pHudPasstime->SetEnabled( bIsPasstime );
	}

	//=============================================================================
	// HPE_END
	//=============================================================================
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudObjectiveStatus::Think()
{
	if ( !TeamplayRoundBasedRules() || !TFGameRules() )
		return;

	SetVisiblePanels();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudObjectiveStatus::ShouldDraw()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		return false;
	}

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	return CHudElement::ShouldDraw();
}
