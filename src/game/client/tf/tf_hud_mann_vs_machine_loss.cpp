//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Wave Loss Screen for MvM
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_hud_mann_vs_machine_loss.h"

using namespace vgui;



const char *g_pszInactiveClassPortraits[] =
{
	"",						// TF_CLASS_UNDEFINED = 0,
	"class_sel_sm_scout_inactive",	
	"class_sel_sm_soldier_inactive",
	"class_sel_sm_pyro_inactive",	
	"class_sel_sm_demo_inactive",	
	"class_sel_sm_heavy_inactive",	
	"class_sel_sm_engineer_inactive",	
	"class_sel_sm_medic_inactive",	
	"class_sel_sm_sniper_inactive",	
	"class_sel_sm_spy_inactive",	
	"class_sel_sm_random_inactive",	 // Random
};

//-----------------------------------------------------------------------------
// CMvMVictoryPanelContainer
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMWaveLossPanel );

CMvMWaveLossPanel::CMvMWaveLossPanel( Panel *parent, const char *pName ) : vgui::EditablePanel( parent, pName )
{
	SetMouseInputEnabled( true );

	MakePopup();
	MoveToFront();

	m_pVoteButton = new CExImageButton( this, "VoteButton", g_pVGuiLocalize->Find( "#TF_PVE_Vote_MissionRestart" ), this );
	m_pCloseButton = new CExImageButton( this, "ContinueButton", g_pVGuiLocalize->Find( "#ConfirmButtonText" ), this );

	m_pHintContainer = NULL;
	m_pCollectionContainer = NULL;
	m_pUsageContainer = NULL;
	m_pCaptainCanteenBody = NULL;
	m_pCaptainCanteenMisc = NULL;
	m_pCaptainCanteenHat = NULL;
	m_pHintImage1 = NULL;
	m_pHintImage2 = NULL;
}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/MvMWaveLossPanel.res" );

	m_pHintContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "HintContainer" ) );
	m_pCollectionContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "CollectionContainer" ) );
	m_pUsageContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "UsageContainer" ) );

	if ( m_pHintContainer )
	{
		m_pCaptainCanteenBody = dynamic_cast<vgui::ImagePanel*>( m_pHintContainer->FindChildByName( "CptCntnBody" ) );
		m_pCaptainCanteenMisc = dynamic_cast<vgui::ImagePanel*>( m_pHintContainer->FindChildByName( "CptCntnMisc" ) );
		m_pCaptainCanteenHat = dynamic_cast<vgui::ImagePanel*>( m_pHintContainer->FindChildByName( "CptCntnHat" ) );

		m_pHintImage1 = dynamic_cast<vgui::ImagePanel*>( m_pHintContainer->FindChildByName( "HintImage1" ) );
		m_pHintImage2 = dynamic_cast<vgui::ImagePanel*>( m_pHintContainer->FindChildByName( "HintImage2" ) );
	}
}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::FireGameEvent( IGameEvent * event )
{

}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::OnTick( void )
{	
	if ( !IsVisible() )
		return;

	if ( TFObjectiveResource() && !TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
	{
		// Close it if the mission starts
		SetMouseInputEnabled( false );
		SetVisible( false );
	}

	// Display Credit Usage Stats
	CMannVsMachineStats *pMVMStats = MannVsMachineStats_GetInstance();
	if ( pMVMStats != NULL )
	{
		if ( m_pCollectionContainer ) 
		{
			m_pCollectionContainer->SetDialogVariable( "creditscollected", (int)pMVMStats->GetAcquiredCredits( -1, false ) );
			m_pCollectionContainer->SetDialogVariable( "creditsmissed", (int)pMVMStats->GetMissedCredits( -1 ) );
			m_pCollectionContainer->SetDialogVariable( "creditbonus", (int)pMVMStats->GetBonusCredits( -1 ) );
		}

		if ( m_pUsageContainer )
		{
			// Local Player
			m_pUsageContainer->SetDialogVariable( "buybacksyou", (int)pMVMStats->GetLocalPlayerBuyBackSpending( -1 ) );
			m_pUsageContainer->SetDialogVariable( "bottlesyou", (int)pMVMStats->GetLocalPlayerBottleSpending( -1 ) );

			int nTotalUpgradeCosts = (int)pMVMStats->GetLocalPlayerUpgradeSpending( -1 );
			CSteamID steamID;
			int nActiveUpgradeCosts = 0;
			if ( C_TFPlayer::GetLocalTFPlayer()->GetSteamID( &steamID ) )
			{
				nActiveUpgradeCosts = (int)pMVMStats->GetPlayerActiveUpgradeCosts( steamID.ConvertToUint64() );
			}

			m_pUsageContainer->SetDialogVariable( "inactiveupgradesyou", nTotalUpgradeCosts - nActiveUpgradeCosts );

			// Team
			nTotalUpgradeCosts = (int)pMVMStats->GetUpgradeSpending();
			nActiveUpgradeCosts = (int)pMVMStats->GetPlayerActiveUpgradeCosts( 0 );

			m_pUsageContainer->SetDialogVariable( "buybacksteam", (int)pMVMStats->GetBuyBackSpending() );
			m_pUsageContainer->SetDialogVariable( "bottlesteam", (int)pMVMStats->GetBottleSpending() );
			m_pUsageContainer->SetDialogVariable( "inactiveupgradesteam", nTotalUpgradeCosts - nActiveUpgradeCosts );
		}
	}

}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "vote_restart" ) )
	{
		engine->ClientCmd( "callvote RestartGame;" );
		SetMouseInputEnabled( false );
		SetVisible( false );
	}
	else if ( !Q_stricmp( command, "continue" )  )
	{
		SetMouseInputEnabled( false );
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::ShowPanel()
{
	SetVisible( true );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( true );
	MoveToFront();

	ClearContents();

	if ( TFObjectiveResource() )
	{
		// Get the WaveNumber
		int iWaveNumber = TFObjectiveResource()->GetMannVsMachineWaveCount();

		wchar_t wszWaveNumber[16];
		_snwprintf( wszWaveNumber, ARRAYSIZE( wszWaveNumber ), L"%d", MAX( 1, iWaveNumber ) );

		wchar_t wszLocalizedWave[512];
		g_pVGuiLocalize->ConstructString_safe( wszLocalizedWave, g_pVGuiLocalize->Find( "#TF_PVE_WaveCountFail" ), 1, wszWaveNumber );
		SetDialogVariable( "waveheader", wszLocalizedWave );
		
		// Pop file
		char szTempName[MAX_PATH];
		V_FileBase( TFObjectiveResource()->GetMvMPopFileName(), szTempName, sizeof( szTempName ) );
		int iChallengeIndex = GetItemSchema()->FindMvmMissionByName( szTempName );

		wchar_t wszLocalizedSummary[ MAX_PATH ];
		if ( GetItemSchema()->GetMvmMissions().IsValidIndex( iChallengeIndex ) )
		{
			const MvMMission_t &mission = GetItemSchema()->GetMvmMissions()[ iChallengeIndex ];
			g_pVGuiLocalize->ConstructString_safe( wszLocalizedSummary, g_pVGuiLocalize->Find( "#TF_PVE_MissionSummaryScheme" ), 2, 
				g_pVGuiLocalize->Find( mission.m_sDisplayName.Get() ), g_pVGuiLocalize->Find( mission.m_sMode.Get() ) );
		}
		else 
		{
			wchar_t wszName[256];
			g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( szTempName ), wszName, sizeof(wszName) );
			g_pVGuiLocalize->ConstructString_safe( wszLocalizedSummary, g_pVGuiLocalize->Find( "#TF_PVE_MissionSummaryDefault" ), 1, 
				wszName );
		}	

		SetDialogVariable( "summaryheader", wszLocalizedSummary );
	}

	
	// Tips
	if ( m_pHintContainer )
	{
		int iClassUsed;
		m_pHintContainer->SetDialogVariable( "hint1", g_TFTips.GetRandomMvMTip( iClassUsed ) );

		bool bShowCaptainCanteen = false;

		if ( m_pHintImage1 )
		{
			SetHintImage( m_pHintImage1, iClassUsed, bShowCaptainCanteen );
		}

		// Make sure we only use captain canteen once
		bool bUsedCaptainCanteen = ( iClassUsed == TF_LAST_NORMAL_CLASS );
		
		m_pHintContainer->SetDialogVariable( "hint2", g_TFTips.GetRandomMvMTip( iClassUsed ) );

		if ( m_pHintImage2 )
		{
			SetHintImage( m_pHintImage2, iClassUsed, bShowCaptainCanteen && !bUsedCaptainCanteen );
		}

		if ( !bUsedCaptainCanteen && iClassUsed != TF_LAST_NORMAL_CLASS )
		{
			// We didn't use captain canteen, hide him
			if ( m_pCaptainCanteenBody )
			{
				m_pCaptainCanteenBody->SetVisible( false );
			}
			if ( m_pCaptainCanteenMisc )
			{
				m_pCaptainCanteenMisc->SetVisible( false );
			}
			if ( m_pCaptainCanteenHat )
			{
				m_pCaptainCanteenHat->SetVisible( false );
			}
		}
	}

	/*waveheader
	summaryheader
	creditscollected
	creditsmissed
	creditbonus

	buybacks
	bottles
	activeupgrades
	inactiveupgrades
	
	hint1
	hint2*/
}

void CMvMWaveLossPanel::SetCaptainCanteenImage( vgui::ImagePanel *panel, const char *pchImage, int nNewX )
{
	if ( !panel )
		return;

	int nX, nY;
	panel->SetVisible( true );
	panel->SetImage( pchImage );
	panel->GetPos( nX, nY );
	panel->SetPos( nNewX, nY );
}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::SetHintImage( vgui::ImagePanel *panel, int iClassUsed, bool bAllowCaptainCanteen )
{
	if ( !panel )
		return;

	if ( bAllowCaptainCanteen && iClassUsed == TF_LAST_NORMAL_CLASS )
	{
		// Hide the normal tip image
		panel->SetVisible( false );

		// Get the asset images
		const char *pchCaptainCanteenBody;
		const char *pchCaptainCanteenMisc;
		const char *pchCaptainCanteenHat;
		g_TFTips.GetRandomCaptainCanteenImages( &pchCaptainCanteenBody, &pchCaptainCanteenMisc, &pchCaptainCanteenHat );

		// Get the destination X position
		int nX, nY;
		panel->GetPos( nX, nY );

		// Set up the canteen images
		if ( m_pCaptainCanteenBody )
		{
			SetCaptainCanteenImage( m_pCaptainCanteenBody, pchCaptainCanteenBody, nX );
		}
		if ( m_pCaptainCanteenMisc )
		{
			SetCaptainCanteenImage( m_pCaptainCanteenMisc, pchCaptainCanteenMisc, nX );
		}
		if ( m_pCaptainCanteenHat )
		{
			SetCaptainCanteenImage( m_pCaptainCanteenHat, pchCaptainCanteenHat, nX );
		}
	}
	else
	{
		panel->SetVisible( true );
		panel->SetImage( g_pszInactiveClassPortraits[ iClassUsed ] );
	}
}

//-----------------------------------------------------------------------------
void CMvMWaveLossPanel::ClearContents()
{
	SetDialogVariable( "waveheader", "" );
	SetDialogVariable( "summaryheader", "" );	

	if ( m_pCollectionContainer ) 
	{
		m_pCollectionContainer->SetDialogVariable( "creditscollected", "" );
		m_pCollectionContainer->SetDialogVariable( "creditsmissed", "" );
		m_pCollectionContainer->SetDialogVariable( "creditbonus", "" );
	}

	if ( m_pUsageContainer )
	{
		m_pUsageContainer->SetDialogVariable( "buybacksyou", "" );
		m_pUsageContainer->SetDialogVariable( "bottlesyou", "" );
		m_pUsageContainer->SetDialogVariable( "activeupgradesyou", "" );
		m_pUsageContainer->SetDialogVariable( "inactiveupgradesyou", "" );

		m_pUsageContainer->SetDialogVariable( "buybacksteam", "" );
		m_pUsageContainer->SetDialogVariable( "bottlesteam", "" );
		m_pUsageContainer->SetDialogVariable( "activeupgradesteam", "" );
		m_pUsageContainer->SetDialogVariable( "inactiveupgradesteam", "" );
	}
	
	// Tips
	if ( m_pHintContainer )
	{
		m_pHintContainer->SetDialogVariable( "hint1", "" );
		m_pHintContainer->SetDialogVariable( "hint2", "" );
	}
}