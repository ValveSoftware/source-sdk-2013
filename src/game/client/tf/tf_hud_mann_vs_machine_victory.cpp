//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Scoreboard for MvM
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_hud_mann_vs_machine_victory.h"
#include "tf_playermodelpanel.h"
#include "econ_item_inventory.h"
#include "vgui/IInput.h"
#include "vgui_controls/PanelListPanel.h"
#include "tf_particlepanel.h"
#include "engine/IEngineSound.h"
#include "econ_notifications.h"
#include "tf_hud_mann_vs_machine_status.h"

using namespace vgui;
extern const ConVar *sv_cheats;

#define MVM_PLAYER_COUNT	6
#define SQUAD_SURPLUS_COUNT 6

//#define WAVE_SUMMARY_TOTAL_TIME		5.0f;
#define CREDITS_COLLECTED_TIME		2.0f
#define CREDITS_MISSED_TIME			1.0f
#define CREDITS_BONUS_TIME			0.5f
#define RATING_LABEL_TIME			0.5f
#define RATING_SCORE_TIME			0.5f
#define SHORT_TIME					0.5f

#define RATING_LABEL_TIME			0.5f
#define RATING_SCORE_TIME			0.5f

#define WAIT_TIME					12.0f

#define STARTING_LOOT_PAUSE_TIME	2.f

// String constants that match variable names in .res files
#define CREDITS_COLLECTED_STR		"creditscollected"
#define CREDITS_MISSED_STR			"creditsmissed"
#define CREDITS_BONUS_STR			"creditbonus"

#define YOUR_UPGRADES_STR			"upgrades"
#define YOUR_BUYBACKS_STR			"buybacks"
#define YOUR_BOTTLES_STR			"bottles"

#define RATING_LABEL_STR			"ratinglabel"
#define RATING_SCORE_STR			"ratingscore"



extern const char *g_szItemBorders[][5];
extern int g_iLegacyClassSelectWeaponSlots[TF_LAST_NORMAL_CLASS];

//-----------------------------------------------------------------------------
// CVictoryPanel
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CVictoryPanel );

CVictoryPanel::CVictoryPanel( Panel *parent, const char *pName )
	: vgui::EditablePanel( parent, pName )
	, m_pHeaderContainer( NULL )
	, m_pCreditContainerPanel( NULL )
	, m_pTotalGameCreditSpendPanel( NULL )
	, m_pTeamStatsContainerPanel( NULL) 
	, m_pYourStatsContainerPanel( NULL) 
	, m_pRatingContainerPanel( NULL) 
	, m_pDoneButton( NULL) 
{
	SetMouseInputEnabled( true );

	m_pDoneButton = new CExImageButton( this, "DoneButton", g_pVGuiLocalize->Find( "#DoneButton" ), this );
}

//-----------------------------------------------------------------------------
void CVictoryPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/MvMVictoryPanel.res" );

	m_pDoneButton->AddActionSignalTarget( GetParent() );

	CExButton *pButton = dynamic_cast<CExButton*>( FindChildByName("DoneButton") );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	vgui::EditablePanel* pStatsContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName("StatsContainer") );
	if ( pStatsContainer )
	{
		m_pHeaderContainer = dynamic_cast<vgui::EditablePanel*>( pStatsContainer->FindChildByName("HeaderContainer") );
		m_pCreditContainerPanel = dynamic_cast<vgui::EditablePanel*>( pStatsContainer->FindChildByName("CreditContainer") );
		m_pRatingContainerPanel = dynamic_cast<vgui::EditablePanel*>( pStatsContainer->FindChildByName("RatingContainer") );
		m_pTotalGameCreditSpendPanel = dynamic_cast<CCreditSpendPanel*>( pStatsContainer->FindChildByName("TotalGameCreditSpendPanel") );
	}

	if ( m_pCreditContainerPanel )
	{
		m_pCreditContainerPanel->SetDialogVariable( "header", "" );
		m_pCreditContainerPanel->SetDialogVariable( "rating", "" );
		m_pCreditContainerPanel->SetDialogVariable( "ratingshadow", "" );
	}
	
	if ( m_pTotalGameCreditSpendPanel )
	{
		m_pTotalGameCreditSpendPanel->SetDialogVariable( "header", "" );
	}
}

//-----------------------------------------------------------------------------
void CVictoryPanel::OnTick( void )
{
	if ( m_eState == FINISHED )
		return;

	m_fStateRunningTime += gpGlobals->curtime - m_fPreviousTick;
	m_fPreviousTick = gpGlobals->curtime;

	// Run through animation loop
	switch ( m_eState )
	{
	case CREDITS_COLLECT:
		StateUpdateValue ( m_pCreditContainerPanel, CREDITS_COLLECTED_STR, CREDITS_COLLECTED_TIME, m_fStateRunningTime, CREDITS_MISSED, m_nCreditsCollected );
		break;
	case CREDITS_MISSED:
		StateUpdateValue ( m_pCreditContainerPanel, CREDITS_MISSED_STR, CREDITS_MISSED_TIME, m_fStateRunningTime, CREDITS_BONUS, m_nCreditsMissed );
		break;
	case CREDITS_BONUS:
		StateUpdateValue ( m_pCreditContainerPanel, CREDITS_BONUS_STR, CREDITS_BONUS_TIME, m_fStateRunningTime, YOUR_UPGRADES, m_nCreditBonus );
		break;
	case YOUR_UPGRADES:
		StateUpdateValue ( m_pTotalGameCreditSpendPanel, YOUR_UPGRADES_STR, SHORT_TIME, m_fStateRunningTime, YOUR_BUYBACK, m_nYourUpgradeCredits );
		break;
	case YOUR_BUYBACK:
		StateUpdateValue ( m_pTotalGameCreditSpendPanel, YOUR_BUYBACKS_STR, SHORT_TIME, m_fStateRunningTime, YOUR_BOTTLES, m_nYourBuybacksCredits );
		break;
	case YOUR_BOTTLES:
		StateUpdateValue ( m_pTotalGameCreditSpendPanel, YOUR_BOTTLES_STR, SHORT_TIME, m_fStateRunningTime, RATING_LABEL, m_nYourBottlesCredits );
		break;
	case RATING_LABEL:
		RatingLabelUpdate();
		CheckState( RATING_LABEL_TIME, m_fStateRunningTime, RATING_SCORE );
		break;
	case RATING_SCORE:
		RatingScoreUpdate();
		CheckState( RATING_SCORE_TIME, m_fStateRunningTime, FINISHED );
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
void CVictoryPanel::ResetVictoryPanel()
{
	m_fStateRunningTime = 0;

	if ( m_pCreditContainerPanel )
	{
		// Set all the values to empty strings
		m_pCreditContainerPanel->SetDialogVariable( CREDITS_COLLECTED_STR, "" );
		m_pCreditContainerPanel->SetDialogVariable( CREDITS_MISSED_STR, "" );
		m_pCreditContainerPanel->SetDialogVariable( CREDITS_BONUS_STR, "" );
	}

	if ( m_pTotalGameCreditSpendPanel )
	{
		m_pTotalGameCreditSpendPanel->SetDialogVariable( YOUR_UPGRADES_STR, "" );
		m_pTotalGameCreditSpendPanel->SetDialogVariable( YOUR_BUYBACKS_STR, "" );
		m_pTotalGameCreditSpendPanel->SetDialogVariable( YOUR_BOTTLES_STR, "" );
	}

	if ( m_pRatingContainerPanel )
	{
		m_pRatingContainerPanel->SetDialogVariable( RATING_LABEL_STR, "" );
		m_pRatingContainerPanel->SetDialogVariable( RATING_SCORE_STR, "" );
	}

	m_eState = CREDITS_COLLECT;
	m_fStateRunningTime = 0;
	m_fPreviousTick = gpGlobals->curtime;

	CaptureStats();
}

//-----------------------------------------------------------------------------
void CVictoryPanel::SetMapAndPopFile ( )
{
	// Map Name
	char szTempMapName[MAX_PATH];
	Q_FileBase( engine->GetLevelName(), szTempMapName, sizeof ( szTempMapName ) );

	wchar_t wszMapName[MAX_PATH];
	g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( szTempMapName ), wszMapName, sizeof(wszMapName) );

	char szTempName[MAX_PATH];
	V_FileBase( TFObjectiveResource()->GetMvMPopFileName(), szTempName, sizeof( szTempName ) );
	int iMissionIndex = GetItemSchema()->FindMvmMissionByName( szTempName );

	wchar_t wszLocalizedSummary[ 256 ];

	if ( GetItemSchema()->GetMvmMissions().IsValidIndex( iMissionIndex ) )
	{
		const MvMMission_t &mission = GetItemSchema()->GetMvmMissions()[ iMissionIndex ];	
		g_pVGuiLocalize->ConstructString_safe( wszLocalizedSummary, L"%s1 : %s2", 2, 
			wszMapName, g_pVGuiLocalize->Find( mission.m_sDisplayName.Get() ) );
	}
	else 
	{
		//Popfile
		wchar_t wszPopFileName[MAX_PATH];
		g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName(szTempName), wszPopFileName, sizeof(wszPopFileName) );

		g_pVGuiLocalize->ConstructString_safe( wszLocalizedSummary, L"%s1 : %s2", 2, 
			wszMapName, wszPopFileName );
	}

	if ( m_pHeaderContainer )
	{
		m_pHeaderContainer->SetDialogVariable( "header", wszLocalizedSummary );
		m_pHeaderContainer->SetDialogVariable( "headershadow", wszLocalizedSummary );
	}
}

//-----------------------------------------------------------------------------
// Purpose : Save all the stats info incase they reset (Lvl reset) while this screen is active
//-----------------------------------------------------------------------------
void CVictoryPanel::CaptureStats()
{
	CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
	if ( !pStats )
		return;

	int nAcquired = pStats->GetAcquiredCredits( -1, false );
	int nDropped = pStats->GetDroppedCredits( -1 );
	int nMissed = nDropped - nAcquired;
	int nBonus = pStats->GetBonusCredits( -1 );

	m_nCreditsCollected = nAcquired;
	m_nCreditsMissed = nMissed;
	m_nCreditBonus = nBonus;

	m_nYourBuybacksCredits = pStats->GetLocalPlayerBuyBackSpending( -1 );
	m_nYourBottlesCredits =  pStats->GetLocalPlayerBottleSpending( -1 );
	m_nYourUpgradeCredits = pStats->GetLocalPlayerUpgradeSpending( -1 );
}

//-----------------------------------------------------------------------------
// Purpose: updates the target field based on the input args.  Returns TRUE if transitioning to new state
//-----------------------------------------------------------------------------
bool CVictoryPanel::StateUpdateValue( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int endValue )
{
	float fPercent = currentTime / targetTime;
	fPercent = 1.0 < fPercent ? 1.0f : fPercent;

	int displayValue = (int)(endValue * fPercent);
	parent->SetDialogVariable( field, displayValue );
	if ( displayValue != endValue )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			pPlayer->EmitSound( "Credits.Updated" );
		}
	}

	// transition to next state
	if ( fPercent >= 1.0f )
	{
		m_fStateRunningTime = 0;
		m_eState = nextState;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: updates the target field based on the input args.  Returns TRUE if transitioning to new state
//	Adds "Credit" count text
//-----------------------------------------------------------------------------
bool CVictoryPanel::StateUpdateCreditText( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int useValue, int creditValue )
{
	float fPercent = currentTime / targetTime;
	fPercent = 1.0 < fPercent ? 1.0f : fPercent;
	int displayValue = (int)(useValue * fPercent);

	char szTmp[32];
	Q_snprintf(szTmp, sizeof(szTmp), "%d (%d Credits)", displayValue, (int)(creditValue * fPercent));
	parent->SetDialogVariable( field, szTmp );

	if ( displayValue != useValue )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			pPlayer->EmitSound( "Credits.Updated" );
		}
	}

	// transition to next state
	if ( fPercent >= 1.0f )
	{
		m_fStateRunningTime = 0;
		m_eState = nextState;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CVictoryPanel::CheckState( float targetTime, float currentTime, int nextState )
{
	if ( currentTime >= targetTime )
	{
		m_fStateRunningTime = 0;
		m_eState = nextState;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVictoryPanel::RatingLabelUpdate( void )
{
	m_pRatingContainerPanel->SetDialogVariable( RATING_LABEL_STR, g_pVGuiLocalize->Find( "#TF_PVE_CreditRating" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVictoryPanel::RatingScoreUpdate( )
{
	//calc a score
	const char* pletterScore = "F";
	
	float fPercent = (float)m_nCreditsCollected / (float)(m_nCreditsCollected + m_nCreditsMissed);

	if ( fPercent >= 1.0 )
	{
		pletterScore = "A+";
	}
	else if ( fPercent >= 0.9 )
	{
		pletterScore = "A";
	}
	else if ( fPercent >= 0.8 )
	{
		pletterScore = "B";
	}
	else if ( fPercent >= 0.7 )
	{
		pletterScore = "C";
	}
	else if ( fPercent >= 0.6 )
	{
		pletterScore = "D";
	}

	m_pRatingContainerPanel->SetDialogVariable( RATING_SCORE_STR, pletterScore );
}