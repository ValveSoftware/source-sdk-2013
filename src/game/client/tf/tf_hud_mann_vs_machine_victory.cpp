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
#include "tf_lobby_server.h"

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



extern const char *g_szItemBorders[AE_MAX_TYPES][5];
extern int g_iLegacyClassSelectWeaponSlots[TF_LAST_NORMAL_CLASS];

class CShowMannUpLootNotification : public CEconNotification
{
public:
	CShowMannUpLootNotification()
	{
		m_pObjective = TFObjectiveResource();
	}

	virtual EType NotificationType() { return eType_AcceptDecline; }
	virtual bool BShowInGameElements() const OVERRIDE { return true; }

	virtual void Accept() OVERRIDE
	{
		CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
		if ( pMannVsMachineStatus && TFObjectiveResource() )
		{
			pMannVsMachineStatus->ReopenVictoryPanel();
		}

		MarkForDeletion();
	}

	virtual void Decline() OVERRIDE
	{
		MarkForDeletion();
	}

	virtual void Trigger() OVERRIDE { Accept(); }
	virtual void UpdateTick() OVERRIDE
	{
		CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
		if ( !pMannVsMachineStatus || !pMannVsMachineStatus->IsVisible() || m_pObjective != TFObjectiveResource() )
		{
			MarkForDeletion();
		}
	}

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast< CShowMannUpLootNotification *>( pNotification ) != NULL; }
	const C_TFObjectiveResource* m_pObjective;

};

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


//-----------------------------------------------------------------------------
// CMvMVictoryMannUpLoot
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMVictoryMannUpLoot );

CMvMVictoryMannUpLoot::CMvMVictoryMannUpLoot( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_pItemModelPanel = new CItemModelPanel( this, "EconItemModel" );
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpLoot::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/MvMVictoryMannUpLoot.res" );
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpLoot::SetEconItem ( CEconItem *econItem )
{
	if ( econItem != NULL )
	{
		m_pItemModelPanel->SetEconItem( econItem );
		m_pItemModelPanel->SetVisible( true );
	}
	else 
	{
		HideEconItem();
	}
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpLoot::HideEconItem ( )
{
	m_pItemModelPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpLoot::SetEconToolTip( CItemModelPanelToolTip *pToolTip)
{
	m_pItemModelPanel->SetTooltip( pToolTip, "" );
}

//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMVictoryMannUpEntry );

CMvMVictoryMannUpEntry::CMvMVictoryMannUpEntry( Panel *parent, const char *pName )
	: vgui::EditablePanel( parent, pName )
	, m_LootLables( DefLessFunc(int) )
	, m_pPlayerModelPanel( NULL )
	, m_bHasData( false )
{
	m_bBadgeUpdated = false;
	m_iProgressWidthStart = 0;
	m_iProgressWidthEnd = 0;

	m_iLootAnimIndex = 0;
	m_flLootAnimTime = 0.f;

	m_pTourProgress = new EditablePanel( this, "TourProgress" );
	m_pProgressBarBG = new EditablePanel( m_pTourProgress, "LevelProgressBarBG" );
	m_pProgressBarFGAnim = new EditablePanel( m_pProgressBarBG, "LevelProgressBarFGAnim" );
	m_pProgressBarFGStatic = new EditablePanel( m_pProgressBarBG, "LevelProgressBarFGStatic" );

	m_pProgressCheckOnBackground = new EditablePanel( this, "MannUpTicketBackground" );
	m_pProgressCheckOn = new vgui::ImagePanel( m_pProgressCheckOnBackground, "CompletedCheckOn" );
	m_pSquadSurplusBackground = new EditablePanel( this, "SquadSurplusTicketBackground" );
	m_pSquadSurplus = new vgui::ImagePanel( m_pSquadSurplusBackground, "SquadSurplus" );

	m_pMissingVoucher = new vgui::Label( this, "MissingVoucher", "" );

#ifdef USE_MVM_TOUR
	m_nChallengeCount = 1;
#else // new mm
	m_nMissionIndex = -1;
#endif // USE_MVM_TOUR

	m_nItemColumns = 1;
	m_nItemXSpacing = 100;
	m_nItemYSpacing = 100;

	m_pItemModelPanelKVs = NULL;
	m_pRowKVs = NULL;
	m_pUnopenedLootKVs = NULL;

	m_pListPanel = new vgui::PanelListPanel( this, "PanelListPanel" );
	m_pListPanel->SetVerticalBufferPixels( 0 );
	m_pListPanel->SetFirstColumnWidth( 0 );

	m_pPlayerModelPanel = new CTFPlayerModelPanel( this, "playermodelpanel" );

	m_LootLables.Purge();
	m_LootLables.Insert( CMsgMvMVictoryInfo_GrantReason_BADGE_LEVELED,	new CExLabel( this, "TourOfDutyLabel",		"" ) );
	m_LootLables.Insert( CMsgMvMVictoryInfo_GrantReason_MANN_UP,		new CExLabel( this, "MannUpLabel",			"" ) );
	m_LootLables.Insert( CMsgMvMVictoryInfo_GrantReason_SQUAD_SURPLUS,	new CExLabel( this, "SquadSurplusLabel",	"" ) );
	m_LootLables.Insert( CMsgMvMVictoryInfo_GrantReason_HELP_A_NOOB,	new CExLabel( this, "VeteranBonusLabel",	"" ) );

	m_pBehindItemParticlePanel = new CTFParticlePanel( this, "BehindItemParticlePanel" );
}

CMvMVictoryMannUpEntry::~CMvMVictoryMannUpEntry()
{
	// Dont let the list panel delete everything it owns.  The labels and dividers
	// technically belong to this panel's buildgroup, which would cause a double free.
	m_pListPanel->RemoveAll();

	ClearPlayerData();

	if ( m_pItemModelPanelKVs )
	{
		m_pItemModelPanelKVs->deleteThis();
		m_pItemModelPanelKVs = NULL;
	}

	if ( m_pRowKVs )
	{
		m_pRowKVs->deleteThis();
		m_pRowKVs = NULL;
	}

	if ( m_pUnopenedLootKVs )
	{
		m_pUnopenedLootKVs->deleteThis();
		m_pUnopenedLootKVs = NULL;
	}
}

void CMvMVictoryMannUpEntry::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "modelpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pItemModelPanelKVs )
		{
			m_pItemModelPanelKVs->deleteThis();
		}
		m_pItemModelPanelKVs = new KeyValues( "modelpanels_kv" );
		pItemKV->CopySubkeys( m_pItemModelPanelKVs );
	}

	KeyValues *pRowKV = inResourceData->FindKey( "rowpanel_kvs" );
	if ( pRowKV )
	{
		if ( m_pRowKVs )
		{
			m_pRowKVs->deleteThis();
		}
		m_pRowKVs = new KeyValues( "rowpanel_kvs" );
		pRowKV->CopySubkeys( m_pRowKVs );
	}

	KeyValues *pUnopenedKV = inResourceData->FindKey( "unopenedPanel_kvs" );
	if ( pUnopenedKV )
	{
		if ( m_pUnopenedLootKVs )
		{
			m_pUnopenedLootKVs->deleteThis();
		}
		m_pUnopenedLootKVs = new KeyValues( "unopenedPanel_kvs" );
		pUnopenedKV->CopySubkeys( m_pUnopenedLootKVs );
	}

	m_nItemColumns = inResourceData->GetInt( "items_columns", 1 );
	m_nItemXSpacing = inResourceData->GetInt( "items_xspacing", 100 );
	m_nItemYSpacing = inResourceData->GetInt( "items_yspacing", 100 );
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/MvMVictoryMannUpEntry.res" );

	UpdatePlayerData();
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::SetItemsToolTip( CItemModelPanelToolTip *pToolTip )
{
	FOR_EACH_VEC( m_vecLootPanels, i )
	{
		m_vecLootPanels[i]->SetTooltip( pToolTip, "" );
	}
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::ClearPlayerData ( )
{
	SetDialogVariable( "name", "");

	if ( m_pSquadSurplus )
		m_pSquadSurplus->SetVisible( false );

	if ( m_pProgressCheckOn )
		m_pProgressCheckOn->SetVisible( false );

	m_pProgressBarBG->SetVisible( false );
	m_pProgressBarFGAnim->SetVisible( false );
	m_pProgressBarFGStatic->SetVisible( false );

	m_pMissingVoucher->SetVisible( false );
	ClearEconItems();
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::UpdatePlayerData()
{
	if ( !m_bHasData )
		return;

	if ( steamapicontext == NULL )
		return;

	m_pListPanel->RemoveAll();
	CSteamID steamID = CSteamID( m_playerData.steam_id() );

	m_hPlayer = GetPlayerBySteamID( steamID );
	// Setup our model panel
	SetModelPanelInfo( ToTFPlayer( m_hPlayer ) );

	SetDialogVariable( "name", steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ) );

	// Reset
	m_pProgressBarBG->SetVisible( true );
	m_pProgressBarFGAnim->SetVisible( true );
	m_pProgressBarFGStatic->SetVisible( true );

	CheckBadgeLevel( m_playerData );

	CTFGSLobby *pLobby = GTFGCClientSystem()->GetLobby();
	if ( pLobby )
	{
		int idxMember = pLobby->GetMemberIndexBySteamID( steamID );
		if ( idxMember >= 0 )
		{
			ConstTFLobbyPlayer member = pLobby->GetMemberDetails( idxMember );
			Assert( member.BMatchPlayer() );
			m_pSquadSurplus->SetVisible( member.GetSquadSurplus() );
		}
	}


	// Loot
	ClearEconItems();
	for( int i = 0; i < m_playerData.items_size(); ++i )
	{
		m_vecLootPanels[ m_vecLootPanels.AddToTail() ] = new CMvMLootItem( this, VarArgs( "modelpanel%d", i ) );
	}


	for ( int iItem = 0; iItem < m_playerData.items_size(); ++iItem )
	{
		const CMsgMvMVictoryInfo_Item& item = m_playerData.items( iItem );
		CEconItem *pEconItem = new CEconItem();
		m_MannUpEconItems.AddToTail( pEconItem );

		if ( pEconItem->BParseFromMessage( item.item_data() ) )
		{
			CMvMLootItem *pLootItem = m_vecLootPanels[ iItem ];
			pLootItem->SetVisible( false );
			pLootItem->SetEconItem( pEconItem );
			pLootItem->m_eReason = item.grant_reason();
		}
		else 
		{
			delete pEconItem;
		}
	}

	// Put the items into a map for ordering them later on
	CUtlMap< int, CCopyableUtlVector<CMvMLootItem*> > mapItems( DefLessFunc(int) );
	FOR_EACH_VEC( m_vecLootPanels, i )
	{
		int nIndex = mapItems.Find( m_vecLootPanels[i]->m_eReason );
		if ( mapItems.InvalidIndex() == nIndex )
		{
			nIndex = mapItems.Insert( m_vecLootPanels[i]->m_eReason );
		}

		mapItems[ nIndex ].AddToTail( m_vecLootPanels[i] );
	}

	bool bAnyThisRow = true;
	vgui::EditablePanel* pItemPanelRow = NULL;

	// The order of the categories
	int nCategories[] = { CMsgMvMVictoryInfo_GrantReason_MANN_UP
						, CMsgMvMVictoryInfo_GrantReason_SQUAD_SURPLUS
						, CMsgMvMVictoryInfo_GrantReason_BADGE_LEVELED };

	// Put the panels into rows in the scrollable panel
	for( int nRow = 0; bAnyThisRow; ++nRow )
	{
		bAnyThisRow = false;
		pItemPanelRow = NULL;

		for( int i = 0; i < ARRAYSIZE( nCategories ); i++ )
		{
			int nMapKey = mapItems.Find( nCategories[i] );
			if ( nMapKey == mapItems.InvalidIndex() )
				continue;

			for( int nColumn = 0; nColumn < m_nItemColumns; ++nColumn )
			{
				int nIndex = ( nRow * m_nItemColumns ) + nColumn;
				if ( nIndex < mapItems[ nMapKey ].Count() )
				{
					bAnyThisRow = true;

					CMvMLootItem *pLootItem = mapItems[ nMapKey ][ nIndex ];

					// Create a row if we need to
					if ( pItemPanelRow == NULL )
					{
						pItemPanelRow = AddLootRow();
					}

					Assert( pItemPanelRow );

					pLootItem->SetParent( pItemPanelRow );
					pLootItem->m_pUnopenedPanel->SetParent( pItemPanelRow );
					pLootItem->m_nIndex = ( i * m_nItemColumns ) + nColumn;
				}
			}
		}
	}

	// Put all the items into our list in the order that we want to reveal them
	m_vecLootPanels.Purge();
	for( int i = 0; i < ARRAYSIZE( nCategories ); i++ )
	{
		int nMapKey = mapItems.Find( nCategories[i] );
		if ( nMapKey == mapItems.InvalidIndex() )
			continue;

		FOR_EACH_VEC( mapItems[ nMapKey ], j )
		{
			m_vecLootPanels.AddToTail( mapItems[ nMapKey ][ j ] );
		}
	}

	// We want at least 5 rows just to fill the space
	while( m_vecRows.Count() < 5 )
	{
		AddLootRow();
	}

	// If player's voucher has gone missing, indicate it
	m_pMissingVoucher->SetVisible( m_playerData.voucher_missing() );

	// Lots of stuff changed
	InvalidateLayout();
}

#ifdef USE_MVM_TOUR
//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::SetPlayerData( const CMsgMvMVictoryInfo_Player& player, int nMissionCount ) 
{
	m_nChallengeCount = nMissionCount;
	m_playerData = player;
	m_bHasData = true;

	UpdatePlayerData();
}
#else // new mm
//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::SetPlayerData( const CMsgMvMVictoryInfo_Player& player, int nMissionIndex ) 
{
	m_nMissionIndex = nMissionIndex;
	m_playerData = player;
	m_bHasData = true;

	UpdatePlayerData();
}
#endif // USE_MVM_TOUR

vgui::EditablePanel* CMvMVictoryMannUpEntry::AddLootRow()
{
	vgui::EditablePanel* pItemPanelRow = new vgui::EditablePanel( m_pListPanel, "itemsrow" ) ;
	m_pListPanel->AddItem( NULL, pItemPanelRow );
	m_vecRows.AddToTail( pItemPanelRow );

	return pItemPanelRow;
}


static const float PROGRESS_ANIM_TIME = 2.0f;
//-----------------------------------------------------------------------------
bool CMvMVictoryMannUpEntry::AnimateProgressBar( void )
{
	if ( IsVisible() == false || m_bBadgeUpdated == false )
	{
		m_pProgressBarFGAnim->SetWide( m_iProgressWidthEnd );
		return true;
	}

	if ( m_flPBarCurrTime > PROGRESS_ANIM_TIME )
	{
		if ( m_bBadgeUpdated == true && m_iProgressWidthEnd == m_pProgressBarBG->GetWide() )
		{
			wchar_t wszTourUp[ 256 ];
			wchar_t wszTourLevel[ 10 ];

			m_pTourProgress->SetDialogVariable( "level", g_pVGuiLocalize->Find( "#TF_MVM_Victory_TourComplete" ) );
			_snwprintf( wszTourLevel, ARRAYSIZE(wszTourLevel) - 1, L"%d", m_nBadgeLevel );

			wszTourLevel[ ARRAYSIZE(wszTourLevel)-1 ] = '\0';
			g_pVGuiLocalize->ConstructString_safe( wszTourUp, g_pVGuiLocalize->Find( "#TF_MvM_TourCount" ), 1, wszTourLevel );
			m_pTourProgress->SetDialogVariable( "level", wszTourUp);
		}

		return true;
	}

	if ( m_flPBarPreviousTime == 0 )
	{
		m_flPBarPreviousTime = gpGlobals->curtime;
	}
	
	m_flPBarCurrTime += gpGlobals->curtime - m_flPBarPreviousTime;
	m_flPBarPreviousTime = gpGlobals->curtime;

	float flRatio = m_flPBarCurrTime / PROGRESS_ANIM_TIME;
	SetBadgeProgressBarProgress( flRatio );

	return false;
}


void CMvMVictoryMannUpEntry::SetBadgeProgressBarProgress( float flPercent )
{
	int nWidth = m_iProgressWidthStart;	
	nWidth += ((float)m_iProgressWidthEnd - (float)m_iProgressWidthStart) * flPercent;
	m_pProgressBarFGAnim->SetWide(nWidth);
}


void CMvMVictoryMannUpEntry::ForceFinishAllAnimation()
{
	// Badge progress bar
	m_flPBarCurrTime = PROGRESS_ANIM_TIME;
	SetBadgeProgressBarProgress( 1.f );

	// Loot
	FOR_EACH_VEC( m_vecLootPanels, i )
	{
		CMvMLootItem* pLootPanel = m_vecLootPanels[ i ];
		pLootPanel->SetVisible( true );
		pLootPanel->m_pUnopenedPanel->SetVisible( false );
	}
}

void CMvMVictoryMannUpEntry::SetLootAnimationPause( float flPause )
{
	m_flLootAnimTime = gpGlobals->curtime + flPause;
	m_flLastLootAnimTime = gpGlobals->curtime + flPause;
}

void CMvMVictoryMannUpEntry::SetActive( bool bActive )
{
	SetVisible( bActive );

	if ( bActive )
	{
		PlayVCD( "class_select" );
	}
}

void CMvMVictoryMannUpEntry::PerformLayout()
{
	BaseClass::PerformLayout();

	FOR_EACH_VEC( m_vecRows, i )
	{
		if ( m_pRowKVs )
		{
			m_vecRows[i]->ApplySettings( m_pRowKVs );
			m_vecRows[i]->SetVisible( true );
			m_vecRows[i]->InvalidateLayout( true, true );
		}
	}

	FOR_EACH_VEC( m_vecLootPanels, i )
	{
		CMvMLootItem* pItemPanel = m_vecLootPanels[i];
		if ( m_pItemModelPanelKVs )
		{
			pItemPanel->ApplySettings( m_pItemModelPanelKVs );
			pItemPanel->InvalidateLayout( true );

			if ( pItemPanel->GetParent() )
			{
				vgui::EditablePanel* pBackground = dynamic_cast<vgui::EditablePanel*>( pItemPanel->GetParent()->FindChildByName( VarArgs( "ItemBackground%d", pItemPanel->m_nIndex + 1 ), true ) );
				if ( pBackground )
				{
					int x,y;
					pBackground->GetPos(x,y);
					pItemPanel->SetPos( x - 5, 2 );

					if ( m_pUnopenedLootKVs )
					{
						pItemPanel->m_pUnopenedPanel->ApplySettings( m_pUnopenedLootKVs );
						pItemPanel->m_pUnopenedPanel->SetVisible( true );
					}

					// The unopened panel is in the same position
					int nXoffset = ( pItemPanel->m_pUnopenedPanel->GetWide() - pItemPanel->GetWide() ) / 2;
					int nYoffset = ( pItemPanel->m_pUnopenedPanel->GetTall() - pItemPanel->GetTall() ) / 2;
					pItemPanel->GetPos( x, y );
					pItemPanel->m_pUnopenedPanel->SetPos( x - nXoffset, y - nYoffset );

		
					// Update unopened panel's image
					pItemPanel->m_pUnopenedPanel->SetImage( CFmtStr( "../backpack/player/items/crafting/prize_crate_%d", RandomInt(1,5) ) );
					
				}
			}
		}
	}

	m_pListPanel->InvalidateLayout( true, false );
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------

bool CMvMVictoryMannUpEntry::AnimTimePassed( float flTime ) const
{
	float flCurTime = gpGlobals->curtime - m_flLootAnimTime;
	float flLastTime = m_flLastLootAnimTime - m_flLootAnimTime;

	return ( flCurTime >= flTime && flLastTime < flTime );
}

bool CMvMVictoryMannUpEntry::AnimateLoot( CTFParticlePanel* pParticlePanel )
{
	bool bDone = AnimateLoot_Internal( pParticlePanel );
	m_flLastLootAnimTime = gpGlobals->curtime;

	return bDone;
}

//-----------------------------------------------------------------------------
bool CMvMVictoryMannUpEntry::AnimateLoot_Internal( CTFParticlePanel *pParticlePanel )
{
	if ( m_vecLootPanels.Count() == 0 )
		return true;

	if ( m_iLootAnimIndex >= m_vecLootPanels.Count() )
	{
		m_iLootAnimIndex = 0;
		return true;
	}

	// Get the loot panel
	CMvMLootItem* pLootPanel = m_vecLootPanels[ m_iLootAnimIndex ];

	if ( pLootPanel == NULL )
	{
		m_iLootAnimIndex++;
		m_flLootAnimTime = gpGlobals->curtime;
		m_flLastLootAnimTime = gpGlobals->curtime;

		return false;
	}

	if ( AnimTimePassed( 0.1f ) )
	{
		// Scroll to it.  We want a little pause in here.
		for( int itemID = m_pListPanel->FirstItem(); itemID != m_pListPanel->InvalidItemID(); itemID = m_pListPanel->NextItem( itemID ) )
		{
			if ( m_pListPanel->GetItemPanel( itemID ) == pLootPanel->GetParent() )
			{
				m_pListPanel->ScrollToItem( itemID );
			}
		}
	}

	// Get loot rarity
	uint32 nRarity = 0;
	static CSchemaAttributeDefHandle pAttrDef_LootRarity( "loot rarity" );
	uint32 nAttribVal = 0;
	if ( pLootPanel->GetItem()->FindAttribute( pAttrDef_LootRarity, &nAttribVal ) )
	{
		nRarity = (int)((float&)nAttribVal);
	}
	Assert( nRarity >= 0 && nRarity <= 2 );
	nRarity = clamp( nRarity, 0, 2 );


	int nPanelXPos, nPanelYPos;
	pLootPanel->GetPos( nPanelXPos, nPanelYPos );

	int nPanelCenterX = nPanelXPos + (pLootPanel->GetWide() / 2);
	int nPanelCenterY = nPanelYPos + (pLootPanel->GetTall() / 2);

	int iItemAbsX, iItemAbsY;

	vgui::ipanel()->GetAbsPos( pLootPanel->GetParent()->GetVPanel(), iItemAbsX, iItemAbsY );

	int x = iItemAbsX + nPanelCenterX;
	int y = iItemAbsY - nPanelCenterY;


	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	static const char* pszParticles [] =
	{
		"mvm_pow_bam",
		"mvm_pow_boing",
		"mvm_pow_crack",
		"mvm_pow_crash",
		"mvm_pow_crit",
		"mvm_pow_poof",
		"mvm_pow_pow",
		"mvm_pow_punch",
		"mvm_pow_smash",
		"mvm_pow_banana",
		"mvm_pow_boot",
		"mvm_pow_loot",
		"mvm_pow_mmph",
		"mvm_pow_caber"
	};

	switch( nRarity )
	{
	case 0:
		{
			if( AnimTimePassed( 0.1f ) )
			{
				if ( pParticlePanel )
				{
					pParticlePanel->FireParticleEffect( pszParticles[ RandomInt(0,ARRAYSIZE(pszParticles)-1)], x, y, 1.25f, false );
				}
			}

			if ( AnimTimePassed( 0.3f ) )
			{
				// Show it
				pLootPanel->SetVisible( true );
				pLootPanel->m_pUnopenedPanel->SetVisible( false );

				if ( pLocalPlayer )
				{
					pLocalPlayer->EmitSound( "ui.cratesmash_common" );
				}

				if ( pLocalPlayer == m_hPlayer )
				{
					engine->ServerCmd( "loot_response common" );
				}

				if ( pParticlePanel )
				{
					pParticlePanel->FireParticleEffect( "mvm_loot_explosion", x, y, 0.4f, false );
				}
			}

			break;
		}
	case 1:
		{
			if ( AnimTimePassed( 0.1f ) )
			{
				if ( pParticlePanel )
				{
					pParticlePanel->FireParticleEffect( "mvm_pow_fuse_movement", x, y, 5.f, false, 5.f );
				}

				if ( pLocalPlayer )
				{
					pLocalPlayer->EmitSound( "ui.cratesmash_rare_long" );
				}
			}

			if ( AnimTimePassed( 2.0f ) )
			{
				// Show it
				pLootPanel->SetVisible( true );
				pLootPanel->m_pUnopenedPanel->SetVisible( false );

				if ( pParticlePanel )
				{
					pParticlePanel->FireParticleEffect( "mvm_pow_gold_seq_firework_mid", x, y, 0.8f, false );
					pParticlePanel->FireParticleEffect( "mvm_loot_explosion", x, y, 0.4f, false );
				}

				if ( pLocalPlayer == m_hPlayer )
				{
					engine->ServerCmd( "loot_response rare" );
				}
			}
			
			break;
		}
	case 2:
	default:	// Above level 2?  Do the super-cool effects
		{
			if ( AnimTimePassed( 0.3f ) )
			{
				if ( pLocalPlayer )
				{
					pLocalPlayer->EmitSound( "ui.cratesmash_ultrarare_long_fireworks" );
				}

				if ( pParticlePanel )
				{
					pParticlePanel->FireParticleEffect( "mvm_pow_gold_seq", x, y, 0.8f, false );
				}
			}

			if ( AnimTimePassed( 3.3f ) )
			{
				// Show it
				pLootPanel->SetVisible( true );
				pLootPanel->m_pUnopenedPanel->SetVisible( false );

				if ( pParticlePanel )
				{
					pParticlePanel->FireParticleEffect( "mvm_loot_explosion", x, y, 0.6f, false );
				}

				if ( m_pBehindItemParticlePanel )
				{
					m_pBehindItemParticlePanel->FireParticleEffect( "mvm_item_godrays_glow", x, y, 5.f, false );
				}

				if ( pLocalPlayer == m_hPlayer )
				{
					engine->ServerCmd( "loot_response ultra_rare" );
				}
			}
			break;
		}
	}

	float flTotalTime[] = { 0.3f, 3.f, 17.f };
	float flEndTime = flTotalTime[nRarity];

	// Add in a pause on the last item if we're not already giving some grand pause
	if ( m_iLootAnimIndex == ( m_vecLootPanels.Count() - 1 ) && ( flEndTime - gpGlobals->curtime ) < 1.f)
	{
		flEndTime += 3.f;
	}

	if ( AnimTimePassed( flEndTime ) )
	{
		// Prime ourselves for the next one
		m_iLootAnimIndex++;
		m_flLootAnimTime = gpGlobals->curtime;
		m_flLastLootAnimTime = gpGlobals->curtime;
	}

	return false;
}

#ifdef USE_MVM_TOUR
//-----------------------------------------------------------------------------
int CMvMVictoryMannUpEntry::GetBadgeCompletionCount ( uint32 iProgressBits )
{
	int nCompleteCount = 0;
	for (int i = 0; i < 32; i++ )
	{
		nCompleteCount += iProgressBits & 1;
		iProgressBits = iProgressBits >> 1;

		if ( iProgressBits == 0 )
		{
			break;
		}
	}

	nCompleteCount = MIN( nCompleteCount, m_nChallengeCount );

	return nCompleteCount;
}
#endif // USE_MVM_TOUR

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::CheckBadgeLevel( const CMsgMvMVictoryInfo_Player& player )
{
	wchar_t wszTourUp[ 256 ];
	wchar_t wszTourLevel[ 10 ];

	m_nBadgeLevel = player.badge_level();
	int nBadgeOffset = player.badge_leveled() ? 1 : 0;
	_snwprintf( wszTourLevel, ARRAYSIZE(wszTourLevel) - 1, L"%d", m_nBadgeLevel - nBadgeOffset );

#ifdef USE_MVM_TOUR
	int count = GetBadgeCompletionCount( player.badge_progress_bits() );

	if ( player.badge_progress_updated() )
	{
		m_bBadgeUpdated = true;
		m_flPBarCurrTime = 0;
		m_flPBarPreviousTime = 0;

		if ( player.badge_leveled() )
		{
			m_iProgressWidthStart =  m_pProgressBarBG->GetWide() * ( (float)(m_nChallengeCount - 1) / (float)m_nChallengeCount );
			m_iProgressWidthEnd =  m_pProgressBarBG->GetWide();
		}
		else
		{
			m_iProgressWidthStart = m_pProgressBarBG->GetWide() * ( (float)(count - 1) / (float)m_nChallengeCount );
			m_iProgressWidthEnd = m_pProgressBarBG->GetWide() * ( (float)(count) / (float)m_nChallengeCount );
		}

		m_pProgressCheckOn->SetVisible( true );
		m_pProgressBarFGStatic->SetWide( m_iProgressWidthStart );
	}
	else
	{
		m_iProgressWidthEnd = m_pProgressBarBG->GetWide() * (float)( (float)count / (float)m_nChallengeCount );
		m_pProgressBarFGStatic->SetWide( m_iProgressWidthEnd );
		
		m_pProgressCheckOn->SetVisible( false );
		m_pProgressBarFGAnim->SetVisible( false );
	}
#else // new mm

	const MvMMission_t& challenge = GetItemSchema()->GetMvmMissions()[m_nMissionIndex];
	if ( player.badge_progress_updated() )
	{
		m_bBadgeUpdated = true;
		m_flPBarCurrTime = 0;
		m_flPBarPreviousTime = 0;

		if ( player.badge_leveled() )
		{
			m_iProgressWidthStart =  m_pProgressBarBG->GetWide() * ( (float)( player.badge_points() - challenge.m_unMannUpPoints ) / (float)k_unMvMMaxPointsPerBadgeLevel );
			m_iProgressWidthEnd =  m_pProgressBarBG->GetWide();
		}
		else
		{
			m_iProgressWidthStart = m_pProgressBarBG->GetWide() * ( (float)( player.badge_points() - challenge.m_unMannUpPoints ) / (float)k_unMvMMaxPointsPerBadgeLevel );
			m_iProgressWidthEnd = m_pProgressBarBG->GetWide() * ( (float)player.badge_points() / (float)k_unMvMMaxPointsPerBadgeLevel );
		}

		m_pProgressCheckOn->SetVisible( true );
		m_pProgressBarFGStatic->SetWide( m_iProgressWidthStart );
	}
	else
	{
		m_iProgressWidthEnd = m_pProgressBarBG->GetWide() * ( (float)player.badge_points() / (float)k_unMvMMaxPointsPerBadgeLevel );
		m_pProgressBarFGStatic->SetWide( m_iProgressWidthEnd );
		
		m_pProgressCheckOn->SetVisible( false );
		m_pProgressBarFGAnim->SetVisible( false );
	}
#endif // USE_MVM_TOUR

	wszTourLevel[ ARRAYSIZE(wszTourLevel)-1 ] = '\0';
	g_pVGuiLocalize->ConstructString_safe( wszTourUp, g_pVGuiLocalize->Find( "#TF_MvM_TourCount" ), 1, wszTourLevel );
	m_pTourProgress->SetDialogVariable( "level", wszTourUp );
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpEntry::ClearEconItems() 
{
	FOR_EACH_VEC ( m_vecLootPanels, i )
	{
		m_vecLootPanels[ i ]->SetItem( NULL );
		m_vecLootPanels[ i ]->SetVisible( false );
	}

	m_vecRows.PurgeAndDeleteElements();
	m_vecLootPanels.Purge();

	// reset
	m_iLootAnimIndex = 0;
	SetLootAnimationPause( 2.f ); // + 2 gives us a pause of 2 seconds when the panel first opens
}


void CMvMVictoryMannUpEntry::PlayVCD( const char * pszVCDName )
{
	if (m_pPlayerModelPanel)
	{
		const int iClass = m_pPlayerModelPanel->GetPlayerClass();
		m_pPlayerModelPanel->PlayVCD(pszVCDName, NULL, false);
		// This causes the VCD to be played.  Yep.
		m_pPlayerModelPanel->HoldItemInSlot(g_iLegacyClassSelectWeaponSlots[iClass]);
	}
}


bool CMvMVictoryMannUpEntry::SetModelPanelInfo( C_TFPlayer* pPlayer )
{
	if ( !pPlayer )
		return false;

	if ( !m_pPlayerModelPanel )
	{
		AssertMsg1( 0, "No model panel in %s", __FUNCTION__ );
		return false;
	}

	CSteamID steamID;
	if ( !pPlayer->GetSteamID( &steamID ) )
	{
		AssertMsg1( 0, "No steamID for user %s", pPlayer->GetPlayerName() );
		m_pPlayerModelPanel->SetVisible( false );
		return false;
	}

	int nClass = pPlayer->GetPlayerClass()->GetClassIndex();
	int nTeam = pPlayer->GetTeamNumber();
	int nLoadoutSlot = g_iLegacyClassSelectWeaponSlots[nClass];	// We want to mirror the class select panel
	CEconItemView *pWeapon = TFInventoryManager()->GetItemInLoadoutForClass( nClass, nLoadoutSlot, &steamID );

	m_pPlayerModelPanel->ClearCarriedItems();
	m_pPlayerModelPanel->SetToPlayerClass( nClass, true );
	m_pPlayerModelPanel->SetTeam( nTeam );

	for ( int wbl = pPlayer->GetNumWearables()-1; wbl >= 0; wbl-- )
	{
		C_TFWearable *pItem = dynamic_cast<C_TFWearable*>( pPlayer->GetWearable( wbl ) );
		if ( !pItem )
			continue;

		if ( pItem->IsViewModelWearable() )
			continue;

		CAttributeContainer *pCont		   = pItem->GetAttributeContainer();
		CEconItemView		*pEconItemView = pCont ? pCont->GetItem() : NULL;

		if ( pEconItemView && pEconItemView->IsValid() )
		{
			m_pPlayerModelPanel->AddCarriedItem( pEconItemView );
		}
	}

	if ( pWeapon )
	{
		m_pPlayerModelPanel->AddCarriedItem( pWeapon );
	}

	m_pPlayerModelPanel->HoldItemInSlot( nLoadoutSlot );
	m_pPlayerModelPanel->SetVisible( true );

	return true;
}

//-----------------------------------------------------------------------------
// CMvMVictoryMannUpPlayerTab
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMVictoryMannUpPlayerTab );

CMvMVictoryMannUpPlayerTab::CMvMVictoryMannUpPlayerTab( Panel *parent, const char *pName )
	: BaseClass( parent, pName )
	, m_pMouseoverHighlightPanel( NULL )
	, m_pActiveTab( NULL )
	, m_bIsActive( false )
	, m_pAvatarImage( NULL )
{
	m_pAvatarImage = new CAvatarImagePanel( this, "PlayerAvatar" );
	m_pMouseoverHighlightPanel = new vgui::EditablePanel( this, "MouseOverTabPanel" );
	m_pActiveTab = new vgui::EditablePanel( this, "ActiveTabPanel" );
}


void CMvMVictoryMannUpPlayerTab::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/MvMVictoryMannUpTab.res" );
}


void CMvMVictoryMannUpPlayerTab::SetPlayer( const CSteamID& steamID )
{
	if ( m_pAvatarImage )
	{
		m_pAvatarImage->SetShouldDrawFriendIcon( false );
		m_pAvatarImage->SetPlayer( steamID, k_EAvatarSize64x64 );
	}
}

void CMvMVictoryMannUpPlayerTab::SetSelected( bool bState )
{
	// Change in state?
	if ( m_bIsActive != bState )
	{
		if ( m_pMouseoverHighlightPanel )
		{
			m_pMouseoverHighlightPanel->SetVisible( false );
		}

		// Becoming the active tab?
		if ( m_pActiveTab )
		{
			m_pActiveTab->SetVisible( bState );
		}
	}

	m_bIsActive = bState;
}

void CMvMVictoryMannUpPlayerTab::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "switch_tab" ) )
	{
		Panel* pParent = GetParent();
		if ( pParent )
		{
			pParent->PostActionSignal( new KeyValues( "Command", "command", VarArgs( "%s_pressed", GetName() ) ) );
		}

		return;
	}
	else if ( !Q_stricmp( command, "highlight_on" ) )
	{
		// Active tab doesnt highlight
		if ( m_bIsActive )
			return;

		if ( m_pMouseoverHighlightPanel )
		{
			m_pMouseoverHighlightPanel->SetVisible( true );
		}
	}
	else if ( !Q_stricmp( command, "highlight_off" ) )
	{
		// Active tab doesnt highlight
		if ( m_bIsActive )
			return;

		if ( m_pMouseoverHighlightPanel )
		{
			m_pMouseoverHighlightPanel->SetVisible( false );
		}
	}

	BaseClass::OnCommand( command );
}



//-----------------------------------------------------------------------------
// CMvMVictoryMannUpPanel
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMVictoryMannUpPanel );

CMvMVictoryMannUpPanel::CMvMVictoryMannUpPanel( Panel *parent, const char *pName )
	: vgui::EditablePanel( parent, pName )
	, m_pNoItemServerContainer( NULL )
{
	SetMouseInputEnabled( true );
	m_hasData = false;

	m_iMannUpLootIndex = 0;
	m_flChangeTabPauseTime = FLT_MAX;

	m_pMouseOverItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mouseoveritempanel" ) );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );
	m_pMouseOverTooltip->SetPositioningStrategy( IPTTP_BOTTOM_SIDE );
	m_pMouseOverItemPanel->MoveToFront();

	m_pDoneButton = new CExImageButton( this, "DoneButton", g_pVGuiLocalize->Find( "#DoneButton" ), this );

	for (int i = 0; i < MVM_PLAYER_COUNT; ++i)
	{
		m_PlayerEntryPanels.AddToTail(new CMvMVictoryMannUpEntry( this, "mannup_entry" ) );
		m_PlayerEntryPanels.Tail()->SetVisible( true );
	}
}


//-----------------------------------------------------------------------------
void CMvMVictoryMannUpPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/MvMVictoryMannUpPanel.res" );

	m_pDoneButton->AddActionSignalTarget( GetParent() );

	vgui::EditablePanel *pContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName("MainPanelContainer") );
	
	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );

	m_vecTabs.Purge();
	m_vecTabButtons.Purge();
	for ( int i = 0; i < MVM_PLAYER_COUNT; ++i )
	{
		CMvMVictoryMannUpPlayerTab *pTab = FindControl<CMvMVictoryMannUpPlayerTab>( VarArgs( "PlayerTab%d", i + 1), true );
		if ( pTab )
		{
			pTab->ApplySchemeSettings( pScheme );
			pTab->SetVisible( false );
			m_vecTabs.AddToTail( pTab );

			vgui::Button* pButton = pTab->FindControl<vgui::Button>( "TabButton", true );
			if ( pButton )
			{
				pButton->SetCommand( CFmtStr( "switch_tab%d", i + 1 ) );
				pButton->AddActionSignalTarget( this );
				m_vecTabButtons.AddToTail( pButton );
			}
		}
	}

	if ( pContainer )
	{
		m_pNoItemServerContainer = dynamic_cast<vgui::EditablePanel*>( pContainer->FindChildByName( "NoItemServerContainer" ) );
	}

	m_pParticlePanel = FindControl<CTFParticlePanel>( "ParticlePanel" );

	LoadVictoryData();
}


//-----------------------------------------------------------------------------
void CMvMVictoryMannUpPanel::OnTick( void )
{
	if ( !IsVisible() )
	{
		return;
	}

	UpdateHighlight();
	
	// Still animating
	if ( !m_bAnimationComplete )
	{
		// If the pause time is set
		if ( m_flChangeTabPauseTime != FLT_MAX )
		{
			// Check if the timeer has passed
			if ( m_flChangeTabPauseTime < gpGlobals->curtime )
			{
				// We're done.  Change the tab.
				m_flChangeTabPauseTime = FLT_MAX;
				SetTabActive( m_iMannUpLootIndex );
				m_PlayerEntryPanels[ m_iMannUpLootIndex ]->SetLootAnimationPause( 0.2f );
			}
			else
			{
				// Has not passed yet.  Dont animate anything.
				return;
			}
		}

		bool bBarComplete = false;
		bool bLootComplete = false;

		// progress bars
		FOR_EACH_VEC ( m_PlayerEntryPanels, i )
		{
			bBarComplete = m_PlayerEntryPanels[i]->AnimateProgressBar();
		}
	
		// Animate loot
		if ( m_iMannUpLootIndex < MVM_PLAYER_COUNT )
		{
			if ( m_PlayerEntryPanels[ m_iMannUpLootIndex ]->AnimateLoot( m_pParticlePanel ) )
			{
				// This entry is done.  Increment the entry index, and set the pause timer.
				// When the timer goes off it will change to the next tab.
				++m_iMannUpLootIndex;
				if ( m_iMannUpLootIndex < MVM_PLAYER_COUNT && m_vecTabs[m_iMannUpLootIndex]->IsVisible() )
				{
					// Slight pause on showing the new tab
					m_flChangeTabPauseTime = gpGlobals->curtime + 0.2f;
				}
				else
				{
					// All loot has been animated
					bLootComplete = true;
				}
			}
		}
		else
		{
			bLootComplete = true;
		}
		
		// The bar and the loot must be complete to be considered totally complete
		m_bAnimationComplete = bBarComplete && bLootComplete;
	}
}

void CMvMVictoryMannUpPanel::SetVisible( bool bState )
{
	BaseClass::SetVisible( bState );

	//int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "global" );

	if ( bState )
	{		
		// Hide all other UI
		//gHUD.LockRenderGroup( iRenderGroup );
	}
	else
	{
		// Let the other UI elements show again
	//	gHUD.UnlockRenderGroup( iRenderGroup );
	}
}

void CMvMVictoryMannUpPanel::UpdateHighlight()
{
	vgui::Panel *pMouseOverPanel = vgui::ipanel()->GetPanel( vgui::input()->GetMouseOver(), "ClientDLL" );

#ifdef DEBUG
	if ( pMouseOverPanel )
	{
		const char * pszParentName = pMouseOverPanel->GetParent() ? pMouseOverPanel->GetParent()->GetName() : "";
		engine->Con_NPrintf( 0, "%s %s", pMouseOverPanel->GetName(), pszParentName );
	}
#endif

	// If we're still animating, fake the that they're not highlighting anything
	if ( !m_bAnimationComplete
		)
	{
		pMouseOverPanel = NULL;
	}

	FOR_EACH_VEC ( m_vecTabButtons, i )
	{
		bool bOverMe = pMouseOverPanel && pMouseOverPanel == m_vecTabButtons[i];
		m_vecTabs[i]->OnCommand( bOverMe? "highlight_on" : "highlight_off" );
	}
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpPanel::ShowVictoryPanel()
{
	SetVisible( true );
	m_pMouseOverItemPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpPanel::ClearData()
{
	// Clear the MvM data
	m_hasData = false;
	// Clear / Reset All Panels
	int iPlayer = 0;
	FOR_EACH_VEC( m_PlayerEntryPanels, i )
	{
		m_PlayerEntryPanels[iPlayer]->ClearPlayerData();
		m_PlayerEntryPanels[iPlayer]->SetVisible( false );
	}

	m_bAnimationComplete = false;
	m_iMannUpLootIndex = 0;
	m_flChangeTabPauseTime = FLT_MAX;

	if ( m_pNoItemServerContainer )
	{
		m_pNoItemServerContainer->SetVisible( true );
	}

	FOR_EACH_VEC( m_vecTabButtons, i )
	{
		m_vecTabButtons[i]->SetVisible( false );
	}

	FOR_EACH_VEC( m_vecTabs, i )
	{
		m_vecTabs[i]->SetVisible( false );
	}
}

void CMvMVictoryMannUpPanel::LoadVictoryData()
{
	if ( !m_hasData )
		return;

	// get the number of challenges in this mission
#ifdef USE_MVM_TOUR
	int nMissionCount = 6;	// default value
	int idxTour = GetItemSchema()->FindMvmTourByName( m_victoryInfo.tour_name().c_str() );
	if ( idxTour >= 0 )
	{
		const MvMTour_t &tour = GetItemSchema()->GetMvmTours()[ idxTour ];
		nMissionCount = tour.m_vecMissions.Count();
	}
#else // new mm
	int nMissionIndex = GetItemSchema()->FindMvmMissionByName( m_victoryInfo.mission_name().c_str() );
	Assert( nMissionIndex >= 0 );
#endif // USE_MVM_TOUR

	// Clear / Reset All Panels
	int iPlayer = 0;
	for ( iPlayer = 0; iPlayer < m_victoryInfo.players_size(); ++iPlayer )
	{
#ifdef USE_MVM_TOUR
		m_PlayerEntryPanels[iPlayer]->SetPlayerData( m_victoryInfo.players( iPlayer ), nMissionCount );
#else // new mm
		m_PlayerEntryPanels[iPlayer]->SetPlayerData( m_victoryInfo.players( iPlayer ), nMissionIndex );
#endif // USE_MVM_TOUR
		m_PlayerEntryPanels[iPlayer]->SetItemsToolTip( m_pMouseOverTooltip );
		// Show the first player
		m_PlayerEntryPanels[iPlayer]->SetVisible( iPlayer == 0 );
		// Setup the tab
		CSteamID steamID = CSteamID( m_victoryInfo.players( iPlayer ).steam_id() );
		if ( iPlayer < m_vecTabs.Count() )
		{
			m_vecTabs[iPlayer]->SetPlayer( steamID );
			m_vecTabs[iPlayer]->SetVisible( true );
		}

		if ( iPlayer < m_vecTabButtons.Count() )
		{
			m_vecTabButtons[iPlayer]->SetVisible( true );
		}
	}

	for ( int iNoPlayer = iPlayer; iNoPlayer < MVM_PLAYER_COUNT; ++iNoPlayer )
	{
		m_PlayerEntryPanels[iNoPlayer]->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
void CMvMVictoryMannUpPanel::MannUpServerResponse( CMsgMvMVictoryInfo &pData )
{
	ClearData();

	m_victoryInfo = pData;
	m_hasData = true;
	m_bAnimationComplete = false;

	LoadVictoryData();

	// Set the first tab active.
	SetTabActive( 0 );

	m_iMannUpLootIndex = 0;

	if ( m_pNoItemServerContainer )
	{
		m_pNoItemServerContainer->SetVisible( false );
	}
}

void CMvMVictoryMannUpPanel::ForceFinishAllAnimation()
{
	m_bAnimationComplete = true;

	FOR_EACH_VEC( m_PlayerEntryPanels, i )
	{
		m_PlayerEntryPanels[i]->ForceFinishAllAnimation();
	}
}


void CMvMVictoryMannUpPanel::SetTabActive( int nIndex )
{
	FOR_EACH_VEC( m_PlayerEntryPanels, i )
	{
		m_PlayerEntryPanels[i]->SetActive( i == nIndex );
		
		if ( i < m_vecTabs.Count() )
		{
			m_vecTabs[i]->SetSelected( i == nIndex );
		}
	}
}

void CMvMVictoryMannUpPanel::OnCommand( const char *command )
{
	if ( !Q_strncmp( command, "switch_tab", ARRAYSIZE("switch_tab") - 1  ) )
	{
		int nIndex = atoi( command + ARRAYSIZE("switch_tab") - 1 ) - 1;

		// Dont allow switching if we're still animating
		if ( m_bAnimationComplete )
		{
			SetTabActive( nIndex );
		}
		
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// CMvMVictoryPanelContainer
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMVictoryPanelContainer );

CMvMVictoryPanelContainer::CMvMVictoryPanelContainer( Panel *parent, const char *pName )
	: vgui::EditablePanel( parent, pName )
{
	SetMouseInputEnabled( true );
	
	m_pVictoryPanelNormal = new CVictoryPanel( this, "VictoryPanelNormal" );
	m_pVictoryPanelMannUp = new CMvMVictoryMannUpPanel( this, "VictoryPanelMannUp" );
	m_pVictoryPanelMannUp->ClearData();
}

//-----------------------------------------------------------------------------
void CMvMVictoryPanelContainer::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/MvMVictoryContainer.res" );

	m_pVictoryPanelNormal->SetVisible( false );
	m_pVictoryPanelMannUp->SetVisible( false );
}

//-----------------------------------------------------------------------------
void CMvMVictoryPanelContainer::FireGameEvent( IGameEvent * event )
{

}

//-----------------------------------------------------------------------------
void CMvMVictoryPanelContainer::OnTick( void )
{	
	if ( !IsVisible() )
		return;

	// The objective changed.  This means we changedlevel
	// or went to the main menu.  Hide everything!
	if ( m_pObjective != TFObjectiveResource() )
	{
		SetVisible( false );
		m_pVictoryPanelMannUp->SetVisible( false );
		m_pVictoryPanelNormal->SetVisible( false );
	}

	if ( m_pVictoryPanelMannUp->IsVisible() )
	{
		m_pVictoryPanelMannUp->OnTick();
	}
	if ( m_pVictoryPanelNormal->IsVisible() )
	{
		m_pVictoryPanelNormal->OnTick();
	}
}

//-----------------------------------------------------------------------------
void CMvMVictoryPanelContainer::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "done" ) )
	{
		SetMouseInputEnabled( false );
		m_pVictoryPanelNormal->SetVisible( false );
		m_pVictoryPanelMannUp->SetVisible( false );
		SetVisible( false );

		// Tell the population manager on the server that we're done viewing the panel
		engine->ServerCmd( "done_viewing_loot" );

		CreateReOpenNotification();
	}
}

void CMvMVictoryPanelContainer::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == STEAMCONTROLLER_A || code == STEAMCONTROLLER_B )
	{
		OnCommand( "done" );
	}
}


void CMvMVictoryPanelContainer::CreateReOpenNotification()
{
	// Only do this for Mann-Up
	CTFGSLobby *pLobby = GTFGCClientSystem()->GetLobby();
	if ( !pLobby || !IsMannUpGroup( pLobby->GetMatchGroup() ) )
	{
		return;
	}

	int iCount = NotificationQueue_Count( &CShowMannUpLootNotification::IsNotificationType );

	if ( iCount == 0 )
	{
		CShowMannUpLootNotification *pNotification = new CShowMannUpLootNotification();
		pNotification->SetText( "#TF_MVM_Victory_Loot_Notification" );
		pNotification->SetLifetime( 10000.0f ); // Last for quite a bit
		NotificationQueue_Add( pNotification );
	}
}

//-----------------------------------------------------------------------------
void CMvMVictoryPanelContainer::ShowVictoryPanel( bool bIsReopening )
{
	SetVisible( true );
	MakePopup();
	MoveToFront();
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( true );
	m_pObjective = TFObjectiveResource();

	m_pVictoryPanelNormal->ResetVictoryPanel();

	// popfile name
	m_pVictoryPanelNormal->SetMapAndPopFile();


	// Which Panel to show
	if ( TFGameRules() && TFGameRules()->GetCurrentMatchGroup() == k_eTFMatchGroup_MvM_MannUp )
	{
		m_pVictoryPanelMannUp->ShowVictoryPanel();

		if ( bIsReopening )
		{
			m_pVictoryPanelMannUp->ForceFinishAllAnimation();
		}
	}
	else
	{
		m_pVictoryPanelNormal->SetVisible( true );
	}
}

