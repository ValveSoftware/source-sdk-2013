
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_pve_winpanel.h"
#include "tf_hud_statpanel.h"
#include "tf_spectatorgui.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include "c_team.h"
#include "tf_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_hud_mann_vs_machine_status.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPVEWinPanel::CTFPVEWinPanel( IViewPort *pViewPort ) : EditablePanel( NULL, "PVEWinPanel" )
{
	//SetAlpha( 0 );
	SetScheme( "ClientScheme" );

	ListenForGameEvent( "pve_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );

	m_bShouldBeVisible = false;

	m_pRespecContainerPanel = NULL;
	m_pRespecBackground = NULL;
	m_pRespecCountLabel = NULL;
	m_pRespecTextLabel = NULL;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 || 
		 Q_strcmp( "teamplay_game_over", pEventName ) == 0 || 
		 Q_strcmp( "tf_game_over", pEventName ) == 0 || 
		 Q_strcmp( "training_complete", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "pve_win_panel", pEventName ) == 0 )
	{
		if ( !g_PR )
			return;

		int iWinningTeam = event->GetInt( "winning_team" );
		int iWinReason = event->GetInt( "winreason" );

		if ( iWinningTeam != TF_TEAM_PVE_INVADERS )
		{
			// Only show this when the robots win
			return;
		}

		LoadControlSettings( "resource/UI/HudPVEWinPanel.res" );	
		InvalidateLayout( false, true );

		SetDialogVariable( "WinningTeamLabel", "" );
		SetDialogVariable( "WinReasonLabel", "" );
		SetDialogVariable( "DetailsLabel", "" );

		wchar_t *pwchWinReason = L"";
		switch ( iWinReason )
		{
		case 0:
		default:
			pwchWinReason = g_pVGuiLocalize->Find( "#Winpanel_PVE_Bomb_Deployed" );
		}

		SetDialogVariable( "WinReasonLabel", pwchWinReason );

		SetDialogVariable( "DetailsLabel", g_pVGuiLocalize->Find( "#TF_PVE_RestoreToCheckpointDetailed" ) );

		m_bShouldBeVisible = true;

		MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudPVEWinPanel.res" );

	m_pRespecBackground = dynamic_cast< vgui::ScalableImagePanel* >( FindChildByName( "RespecBackground" ) );
	m_pRespecContainerPanel = dynamic_cast< vgui::EditablePanel* >( FindChildByName( "RespecContainer" ) );
	if ( m_pRespecContainerPanel )
	{
		m_pRespecTextLabel = dynamic_cast< vgui::Label* >( m_pRespecContainerPanel->FindChildByName( "RespecTextLabelLoss" ) );
		m_pRespecCountLabel = dynamic_cast< vgui::Label* >( m_pRespecContainerPanel->FindChildByName( "RespecCountLabel" ) );
	}
}

void CTFPVEWinPanel::OnTick()
{
	if ( m_bShouldBeVisible == true )
	{
		IViewPortPanel *scoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
		if ( (scoreboard && scoreboard->IsVisible() == true) || IsInFreezeCam() == true )
		{
			SetVisible( false );
			return;
		}

		// We dont want the stats panel showing up at the same time
		CTFStatPanel *pStatPanel = GetStatPanel();
		if ( pStatPanel && pStatPanel->IsVisible() )
		{
			pStatPanel->Hide();
		}

		if ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			m_bShouldBeVisible = false;
		}

		// Respec
		if ( m_pRespecContainerPanel && m_pRespecBackground && m_pRespecCountLabel && m_pRespecTextLabel )
		{
			CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
			if ( pStats )
			{
				uint16 nRespecs = pStats->GetNumRespecsEarnedInWave();

				bool bRespecVisible = nRespecs > 0;

				// Do this only once
				if ( bRespecVisible && !m_pRespecBackground->IsVisible() )
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "RespecEarnedPulseLoss" );
				
					C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
					if ( pLocalTFPlayer )
					{
						pLocalTFPlayer->EmitSound( "MVM.RespecAwarded" );
					}
				}

				if ( m_pRespecContainerPanel->IsVisible() != bRespecVisible )
				{
					m_pRespecContainerPanel->SetVisible( bRespecVisible );
				}

				if (  m_pRespecBackground->IsVisible() != bRespecVisible )
				{
					m_pRespecBackground->SetVisible( bRespecVisible );
				}

				if ( m_pRespecCountLabel->IsVisible() != bRespecVisible )
				{
					m_pRespecCountLabel->SetVisible( bRespecVisible );
				}

				if ( m_pRespecTextLabel->IsVisible() != bRespecVisible )
				{
					m_pRespecTextLabel->SetVisible( bRespecVisible );
				}

				if ( bRespecVisible )
				{
					m_pRespecContainerPanel->SetDialogVariable( "respeccount", nRespecs );
				}
			}
		}
	}

	SetVisible( m_bShouldBeVisible );
}

void CTFPVEWinPanel::Reset( void )
{
	Update();
	m_bShouldBeVisible = false;
}

void CTFPVEWinPanel::Update( void )
{
}
