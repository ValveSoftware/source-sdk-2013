
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "hud.h"
#include "tf_match_summary.h"
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
#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include "tf_playermodelpanel.h"
#include "tf_mapinfo.h"
#include "c_tf_team.h"
#include "tf_pvp_rank_panel.h"
#include "tf_badge_panel.h"
#include "tf_survey_questions.h"
#include "tf_ladder_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
extern ISoundEmitterSystemBase *soundemitterbase;

#define MAX_PLAYER_MODELS	6

#define MS_STATE_TRANSITION_TO_STATS					17.0f
#define MS_STATE_TRANSITION_TO_MEDALS					3.0f
#define MS_STATE_TIME_BETWEEN_MEDALS					0.1f
#define MS_STATE_TIME_BETWEEN_MEDALS_CATEGORIES			0.1f

extern ConVar tf_scoreboard_alt_class_icons;

DECLARE_BUILD_FACTORY( TFSectionedListPanel );

DECLARE_HUDELEMENT( CTFMatchSummary );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMatchSummary::CTFMatchSummary( const char *pElementName ) 
	: CHudElement( pElementName )
	, EditablePanel( NULL, "MatchSummary" )
	, m_bXPShown( false )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pMainStatsContainer = new EditablePanel( this, "MainStatsContainer" );
	m_pDrawingPanel = new CDrawingPanel( this, "DrawingPanel" );
	m_pStatsBgPanel = new EditablePanel( this, "StatsBgPanel" );

	m_pTeamScoresPanel = new EditablePanel( m_pMainStatsContainer, "TeamScoresPanel" );
	m_pParticlePanel = new CTFParticlePanel( m_pMainStatsContainer, "ParticlePanel" );
	m_pStatsLabelPanel = new EditablePanel( m_pMainStatsContainer, "StatsLabelPanel" );

	m_pBlueTeamPanel = new EditablePanel( m_pTeamScoresPanel, "BlueTeamPanel" );
	m_pRedTeamPanel = new EditablePanel( m_pTeamScoresPanel, "RedTeamPanel" );

	m_pPlayerListBlueParent = new EditablePanel( m_pBlueTeamPanel, "BluePlayerListParent" );
	m_pPlayerListBlue = new TFSectionedListPanel( m_pPlayerListBlueParent, "BluePlayerList" );
	m_pPlayerListRedParent = new EditablePanel( m_pRedTeamPanel, "RedPlayerListParent" );
	m_pPlayerListRed = new TFSectionedListPanel( m_pPlayerListRedParent, "RedPlayerList" );

	m_pBlueTeamScore = new CExLabel( m_pBlueTeamPanel, "BlueTeamScore", "" );
	m_pBlueTeamScoreDropshadow = new CExLabel( m_pBlueTeamPanel, "BlueTeamScoreDropshadow", "" );
	m_pBlueTeamScoreBG = new EditablePanel( m_pBlueTeamPanel, "BlueTeamScoreBG" );
	m_pBluePlayerListBG = new EditablePanel( m_pBlueTeamPanel, "BluePlayerListBG" );
	m_pRedTeamScore = new CExLabel( m_pRedTeamPanel, "RedTeamScore", "" );
	m_pRedTeamScoreDropshadow = new CExLabel( m_pRedTeamPanel, "RedTeamScoreDropshadow", "" );
	m_pRedTeamScoreBG = new EditablePanel( m_pRedTeamPanel, "RedTeamScoreBG" );
	m_pRedPlayerListBG = new EditablePanel( m_pRedTeamPanel, "RedPlayerListBG" );
	m_pBlueMedalsPanel = new EditablePanel( m_pTeamScoresPanel, "BlueMedals" );
	m_pRedMedalsPanel = new EditablePanel( m_pTeamScoresPanel, "RedMedals" );
	m_pRedTeamImage = new vgui::ImagePanel( m_pRedTeamPanel, "RedTeamImage" );
	m_pBlueTeamImage = new vgui::ImagePanel( m_pBlueTeamPanel, "BlueTeamImage" );
	m_pRedLeaderAvatarImage = new CAvatarImagePanel( m_pRedTeamPanel, "RedLeaderAvatar" );
	m_pBlueLeaderAvatarImage = new CAvatarImagePanel( m_pBlueTeamPanel, "BlueLeaderAvatar" );
	m_pRedLeaderAvatarBG = new EditablePanel( m_pRedTeamPanel, "RedLeaderAvatarBG" );
	m_pBlueLeaderAvatarBG = new EditablePanel( m_pBlueTeamPanel, "BlueLeaderAvatarBG" );
	m_pStatsAndMedals = new CExLabel( m_pStatsLabelPanel, "StatsAndMedals", "" );
	m_pStatsAndMedalsShadow = new CExLabel( m_pStatsLabelPanel, "StatsAndMedalsShadow", "" );
	m_pRedTeamName = new CExLabel( m_pRedTeamPanel, "RedTeamLabel", "" );
	m_pBlueTeamName = new CExLabel( m_pBlueTeamPanel, "BlueTeamLabel", "" );
	m_pRedTeamWinner = new CExLabel( m_pRedTeamPanel, "RedTeamWinner", "" );
	m_pRedTeamWinnerDropshadow = new CExLabel( m_pRedTeamPanel, "RedTeamWinnerDropshadow", "" );
	m_pBlueTeamWinner = new CExLabel( m_pBlueTeamPanel, "BlueTeamWinner", "" );
	m_pBlueTeamWinnerDropshadow = new CExLabel( m_pBlueTeamPanel, "BlueTeamWinnerDropshadow", "" );

	m_pImageList = NULL;

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapAvatarsToImageList.RemoveAll();

	Q_memset( m_SkillRatings, 0, sizeof( m_SkillRatings ) );

	m_iCurrentState = MS_STATE_INITIAL;
	m_flNextActionTime = -1;

	m_nMedalsToAward_Bronze_Blue = 0;
	m_nMedalsToAward_Silver_Blue = 0;
	m_nMedalsToAward_Gold_Blue = 0;
	m_nMedalsToAward_Bronze_Red = 0;
	m_nMedalsToAward_Silver_Red = 0;
	m_nMedalsToAward_Gold_Red = 0;
	
	m_nMedalsRevealed = 0;
	m_nNumMedalsThisUpdate = 0;

	m_bBlueGoldValueRevealed = false;
	m_bBlueSilverValueRevealed = false;
	m_bBlueBronzeValueRevealed = false;
	m_bRedGoldValueRevealed = false;
	m_bRedSilverValueRevealed = false;
	m_bRedBronzeValueRevealed = false;
	m_bPlayerAbandoned = false;

	m_flMedalSoundTime = -1.f;

	m_bLargeMatchGroup = false;

	Q_memset( m_iImageClass, NULL, sizeof( m_iImageClass ) );
	Q_memset( m_iImageClassAlt, NULL, sizeof( m_iImageClassAlt ) );

	ListenForGameEvent( "competitive_victory" );
	ListenForGameEvent( "competitive_stats_update" );
	ListenForGameEvent( "player_abandoned_match" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "show_match_summary" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMatchSummary::~CTFMatchSummary()
{
	if ( NULL != m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFMatchSummary::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bLargeMatchGroup = false;

	KeyValues *pConditions = NULL;
	if ( TFGameRules() )
	{
		const IMatchGroupDescription* pMatch = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( pMatch )
		{
			auto lambdaAddCondition = [ &pConditions ]( const char* pszCondition )
			{
				if ( !pConditions )
					pConditions = new KeyValues( "conditions" );
				AddSubKeyNamed( pConditions, pszCondition );
			};

			if ( pMatch->GetMatchSize() > 12 )
			{
				lambdaAddCondition( "if_large" );
				m_bLargeMatchGroup = true;
			}

			if ( pMatch->BUsesPlacementMatches() )
			{
				lambdaAddCondition( "if_uses_placement" );
			}
			
			if ( pMatch->BUsesXP() )
			{
				lambdaAddCondition( "if_uses_xp" );
			}
		}
	}

	LoadControlSettings( "resource/UI/HudMatchSummary.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	if ( m_pDrawingPanel )
	{
		m_pDrawingPanel->ClearAllLines();
		m_pDrawingPanel->SetType( DRAWING_PANEL_TYPE_MATCH_SUMMARY );
		m_pDrawingPanel->MakePopup();
	}


	if ( m_pImageList )
		delete m_pImageList;
	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();

	for ( int i = 1; i < (int)StatMedal_Max; i++ )
	{
		m_iImageMedals[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszCompetitiveMedalImages[i], true ) );
	}

	for ( int i = 1; i < SCOREBOARD_CLASS_ICONS; i++ )
	{
		m_iImageClass[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIcons[i], true ) );
		m_iImageClassAlt[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIconsAlt[i], true ) );
	}

	int iCurrentCount = m_pImageList->GetImageCount();

	// resize the images to our resolution
	for ( int i = 0; i < iCurrentCount; i++ )
	{
		int wide = 13, tall = 13;
		m_pImageList->GetImage( i )->SetSize( scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(), tall ) );
	}

	// resize the images to our resolution
	for ( int i = iCurrentCount; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide = 26, tall = 26;
		m_pImageList->GetImage( i )->SetSize( scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(), tall ) );
	}

	SetPaintBackgroundEnabled( false );
	m_pTeamScoresPanel->SetPaintBackgroundEnabled( false );
	m_pPlayerListBlueParent->SetPaintBackgroundEnabled( false );
	m_pPlayerListRedParent->SetPaintBackgroundEnabled( false );
	m_pPlayerListBlue->SetPaintBackgroundEnabled( false );
	m_pPlayerListRed->SetPaintBackgroundEnabled( false );

	m_pPlayerListBlue->SetImageList( m_pImageList, false );
	m_pPlayerListBlue->SetVisible( true );

	m_pPlayerListRed->SetImageList( m_pImageList, false );
	m_pPlayerListRed->SetVisible( true );

	InitPlayerList( m_pPlayerListBlue, TF_TEAM_BLUE );
	InitPlayerList( m_pPlayerListRed, TF_TEAM_RED );

	m_hFont = pScheme->GetFont( "ScoreboardVerySmall", true );

	m_iCurrentState = MS_STATE_INITIAL;
	m_flNextActionTime = -1;

	RecalculateMedalCounts();

	m_nMedalsRevealed = 0;

	m_bBlueGoldValueRevealed = false;
	m_bBlueSilverValueRevealed = false;
	m_bBlueBronzeValueRevealed = false;
	m_bRedGoldValueRevealed = false;
	m_bRedSilverValueRevealed = false;
	m_bRedBronzeValueRevealed = false;

	m_flMedalSoundTime = -1.f;
	m_flDrawingPanelTime = -1.f;

	m_pBlueMedalsPanel->SetDialogVariable( "blueteammedals_gold", "?" );
	m_pBlueMedalsPanel->SetDialogVariable( "blueteammedals_silver", "?" );
	m_pBlueMedalsPanel->SetDialogVariable( "blueteammedals_bronze", "?" );
	m_pRedMedalsPanel->SetDialogVariable( "redteammedals_gold", "?" );
	m_pRedMedalsPanel->SetDialogVariable( "redteammedals_silver", "?" );
	m_pRedMedalsPanel->SetDialogVariable( "redteammedals_bronze", "?" );

	Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::PerformLayout()
{
	BaseClass::PerformLayout();

	EditablePanel* pStatsContainer = FindControl< EditablePanel >( "MainStatsContainer" );
	if ( pStatsContainer && m_bLargeMatchGroup )
	{
		pStatsContainer->SetPos( pStatsContainer->GetXPos(), m_iAnimStatsContainer12v12YPos );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMatchSummary::ShouldDraw( void )
{
	return IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::SetVisible( bool state )
{
	int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "mid" );

	if ( state )
	{
		gHUD.LockRenderGroup( iRenderGroup );

		InvalidateLayout( true, true );

		m_iCurrentState = MS_STATE_INITIAL;

		m_flDrawingPanelTime = gpGlobals->curtime + 4.5f;

		CPvPRankPanel* pPvPRankPanel = FindControl< CPvPRankPanel >( "RankPanel" );
		if ( pPvPRankPanel )
		{
			pPvPRankPanel->SetMatchGroup( TFGameRules()->GetCurrentMatchGroup() );
		}

		pPvPRankPanel = FindControl< CPvPRankPanel >( "RankModelPanel" );
		if ( pPvPRankPanel )
		{
			pPvPRankPanel->SetMatchGroup( TFGameRules()->GetCurrentMatchGroup() );
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "CompetitiveGame_LowerChatWindow", false );
	}
	else
	{
		gHUD.UnlockRenderGroup( iRenderGroup );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CTFMatchSummary::TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
{
	KeyValues *it1 = list->GetItemData( itemID1 );
	KeyValues *it2 = list->GetItemData( itemID2 );
	Assert( it1 && it2 );

	// first compare score
	int v1 = it1->GetInt( "score" );
	int v2 = it2->GetInt( "score" );
	if ( v1 > v2 )
		return true;
	else if ( v1 < v2 )
		return false;

	// if score is the same, use player index to get deterministic sort
	int iPlayerIndex1 = it1->GetInt( "playerIndex" );
	int iPlayerIndex2 = it2->GetInt( "playerIndex" );
	return ( iPlayerIndex1 > iPlayerIndex2 );
}

//-----------------------------------------------------------------------------
// Purpose: Inits the player list in a list panel
//-----------------------------------------------------------------------------
void CTFMatchSummary::InitPlayerList( TFSectionedListPanel *pPlayerList, int nTeam )
{
	float flAspectRatio = engine->GetScreenAspectRatio();
	bool bStandard = flAspectRatio < 1.6f;

	pPlayerList->SetVerticalScrollbar( false );
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPlayerList->SetSectionAlwaysVisible( 0, true );
	pPlayerList->SetSectionDrawDividerBar( 0, false );
	pPlayerList->SetBorder( NULL );
	pPlayerList->SetMouseInputEnabled( false );
	pPlayerList->SetClickable( false );

	pPlayerList->AddColumnToSection( 0, "medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, pPlayerList->m_iMedalWidth );
	pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iAvatarWidth );
	pPlayerList->AddColumnToSection( 0, "spacer", "", 0, pPlayerList->m_iSpacerWidth );
	pPlayerList->AddColumnToSection( 0, "name", "", 0, pPlayerList->m_iNameWidth );
	pPlayerList->AddColumnToSection( 0, "class", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iClassWidth );
	pPlayerList->AddColumnToSection( 0, "score", bStandard ? "#TF_Comp_Scoreboard_Score_Standard" : "#TF_Comp_Scoreboard_Score", SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iStatsWidth );
	pPlayerList->AddColumnToSection( 0, "score_medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iAwardWidth );
	pPlayerList->AddColumnToSection( 0, "kills", bStandard ? "#TF_Comp_Scoreboard_Kills_Standard" : "#TF_Comp_Scoreboard_Kills", SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iStatsWidth );
	pPlayerList->AddColumnToSection( 0, "kills_medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iAwardWidth );
	pPlayerList->AddColumnToSection( 0, "damage", bStandard ? "#TF_Comp_Scoreboard_Damage_Standard" : "#TF_Comp_Scoreboard_Damage", SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iStatsWidth );
	pPlayerList->AddColumnToSection( 0, "damage_medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iAwardWidth );
	pPlayerList->AddColumnToSection( 0, "healing", bStandard ? "#TF_Comp_Scoreboard_Healing_Standard" : "#TF_Comp_Scoreboard_Healing", SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iStatsWidth );
	pPlayerList->AddColumnToSection( 0, "healing_medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iAwardWidth );
	pPlayerList->AddColumnToSection( 0, "support", bStandard ? "#TF_Comp_Scoreboard_Support_Standard" : "#TF_Comp_Scoreboard_Support", SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iStatsWidth );
	pPlayerList->AddColumnToSection( 0, "support_medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, pPlayerList->m_iAwardWidth );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::Update( void )
{
	UpdateTeamInfo();
	UpdatePlayerList();
	UpdateBadgePanels( m_pRedBadgePanels, m_pPlayerListRed );
	UpdateBadgePanels( m_pBlueBadgePanels, m_pPlayerListBlue );
}

//-----------------------------------------------------------------------------
// Purpose: Updates information about teams
//-----------------------------------------------------------------------------
void CTFMatchSummary::UpdateTeamInfo()
{
	bool bUseWinnerLabel = false;
	if ( GetGlobalTFTeam( TF_TEAM_RED ) && GetGlobalTFTeam( TF_TEAM_BLUE ) )
	{
		if ( GetGlobalTFTeam( TF_TEAM_RED )->Get_Score() == GetGlobalTFTeam( TF_TEAM_BLUE )->Get_Score() )
		{
			bUseWinnerLabel = true;
		}
	}

	int nWinningTeam = TEAM_INVALID;
	if ( TFGameRules() )
	{
		nWinningTeam = TFGameRules()->GetWinningTeam();
	}

	for ( int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_BLUE; teamIndex++ )
	{
		C_TFTeam *team = GetGlobalTFTeam( teamIndex );
		if ( team )
		{
			// choose dialog variables to set depending on team
			const char *pDialogVarTeamName = "";
			const char *pDialogVarTeamScore = "";
			const char *pDialogVarWinner = "";
			vgui::EditablePanel *pOwner = NULL;

			switch ( teamIndex )
			{
			case TF_TEAM_RED:
				pDialogVarTeamName = "redteamname";
				pDialogVarTeamScore = "redteamscore";
				pDialogVarWinner = "redteamwinner";
				pOwner = m_pRedTeamPanel;
				break;
			case TF_TEAM_BLUE:
				pDialogVarTeamName = "blueteamname";
				pDialogVarTeamScore = "blueteamscore";
				pDialogVarWinner = "blueteamwinner";
				pOwner = m_pBlueTeamPanel;
				break;
			default:
				Assert( false );
				break;
			}

			if ( !pOwner )
				return;

			// set the team name
			pOwner->SetDialogVariable( pDialogVarTeamName, team->Get_Localized_Name() );

			if ( bUseWinnerLabel )
			{
				const char *pszLabel = "";
				if ( teamIndex == nWinningTeam ) 
				{
					if ( team->GetNumPlayers() > 1 )
					{
						pszLabel = "#TF_Winners";
					}
					else
					{
						pszLabel = "#TF_Winner";
					}
				}

				pOwner->SetDialogVariable( pDialogVarTeamScore, "" );
				pOwner->SetDialogVariable( pDialogVarWinner, g_pVGuiLocalize->Find( pszLabel ) );
			}
			else
			{
				pOwner->SetDialogVariable( pDialogVarTeamScore, team->Get_Score() );
				pOwner->SetDialogVariable( pDialogVarWinner, "" );
			}
		}
	}

	bool bShowAvatars = g_TF_PR && g_TF_PR->HasPremadeParties();

	if ( bShowAvatars )
	{
		m_pRedLeaderAvatarImage->SetPlayer( GetSteamIDForPlayerIndex( g_TF_PR->GetPartyLeaderRedTeamIndex() ), k_EAvatarSize64x64 );
		m_pRedLeaderAvatarImage->SetShouldDrawFriendIcon( false );
		m_pBlueLeaderAvatarImage->SetPlayer( GetSteamIDForPlayerIndex( g_TF_PR->GetPartyLeaderBlueTeamIndex() ), k_EAvatarSize64x64 );
		m_pBlueLeaderAvatarImage->SetShouldDrawFriendIcon( false );
	}

	m_pRedLeaderAvatarImage->SetVisible( bShowAvatars );
	m_pRedLeaderAvatarBG->SetVisible( bShowAvatars );
	m_pRedTeamName->SetVisible( bShowAvatars );
	m_pRedTeamImage->SetVisible( !bShowAvatars );

	m_pBlueLeaderAvatarImage->SetVisible( bShowAvatars );
	m_pBlueLeaderAvatarBG->SetVisible( bShowAvatars );
	m_pBlueTeamName->SetVisible( bShowAvatars );
	m_pBlueTeamImage->SetVisible( !bShowAvatars );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the last medal (column) added so we can display some effects
//-----------------------------------------------------------------------------
matchsummary_columns_t CTFMatchSummary::InternalAddMedalKeyValues( int iIndex, StatMedal_t eMedal, KeyValues *pKeyValues, int nTotalMedals /*= -1*/ )
{	
	int nMedal = (int)eMedal;
	matchsummary_columns_t retVal = MS_COLUMN_INVALID;
	
	if ( !IsIndexIntoPlayerArrayValid( iIndex ) )
		return retVal;

	if ( ( nTotalMedals < 0 ) || ( m_nNumMedalsThisUpdate <= nTotalMedals ) )
	{
		if ( m_SkillRatings[iIndex].nScoreRank == nMedal )
		{
			pKeyValues->SetInt( "score_medal", m_iImageMedals[nMedal] );
			if ( nTotalMedals >= 0 )
			{
				m_nNumMedalsThisUpdate++;
			}

			if ( ( nTotalMedals >= 0 ) && ( m_nNumMedalsThisUpdate > nTotalMedals ) )
			{
				retVal = MS_COLUMN_SCORE_MEDAL;
			}
		}
	}

	if ( ( nTotalMedals < 0 ) || ( m_nNumMedalsThisUpdate <= nTotalMedals ) )
	{
		if ( m_SkillRatings[iIndex].nKillsRank == nMedal )
		{
			pKeyValues->SetInt( "kills_medal", m_iImageMedals[nMedal] );
			if ( nTotalMedals >= 0 )
			{
				m_nNumMedalsThisUpdate++;
			}

			if ( ( nTotalMedals >= 0 ) && ( m_nNumMedalsThisUpdate > nTotalMedals ) )
			{
				retVal = MS_COLUMN_KILLS_MEDAL;
			}
		}
	}

	if ( ( nTotalMedals < 0 ) || ( m_nNumMedalsThisUpdate <= nTotalMedals ) )
	{
		if ( m_SkillRatings[iIndex].nDamageRank == nMedal )
		{
			pKeyValues->SetInt( "damage_medal", m_iImageMedals[nMedal] );
			if ( nTotalMedals >= 0 )
			{
				m_nNumMedalsThisUpdate++;
			}

			if ( ( nTotalMedals >= 0 ) && ( m_nNumMedalsThisUpdate > nTotalMedals ) )
			{
				retVal = MS_COLUMN_DAMAGE_MEDAL;
			}
		}
	}

	if ( ( nTotalMedals < 0 ) || ( m_nNumMedalsThisUpdate <= nTotalMedals ) )
	{
		if ( m_SkillRatings[iIndex].nHealingRank == nMedal )
		{
			pKeyValues->SetInt( "healing_medal", m_iImageMedals[nMedal] );
			if ( nTotalMedals >= 0 )
			{
				m_nNumMedalsThisUpdate++;
			}

			if ( ( nTotalMedals >= 0 ) && ( m_nNumMedalsThisUpdate > nTotalMedals ) )
			{
				retVal = MS_COLUMN_HEALING_MEDAL;
			}
		}
	}

	if ( ( nTotalMedals < 0 ) || ( m_nNumMedalsThisUpdate <= nTotalMedals ) )
	{
		if ( m_SkillRatings[iIndex].nSupportRank == nMedal )
		{
			pKeyValues->SetInt( "support_medal", m_iImageMedals[nMedal] );
			if ( nTotalMedals >= 0 )
			{
				m_nNumMedalsThisUpdate++;
			}

			if ( ( nTotalMedals >= 0 ) && ( m_nNumMedalsThisUpdate > nTotalMedals ) )
			{
				retVal = MS_COLUMN_SUPPORT_MEDAL;
			}
		}
	}

	return retVal;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFMatchSummary::UpdatePlayerList()
{
	m_pPlayerListRed->RemoveAll();
	m_pPlayerListRed->ClearAllColorOverrideForCell();

	m_pPlayerListBlue->RemoveAll();
	m_pPlayerListBlue->ClearAllColorOverrideForCell();

	if ( !g_TF_PR )
		return;

	m_nNumMedalsThisUpdate = 0;

	for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( g_PR->IsConnected( playerIndex ) || g_PR->IsValid( playerIndex ) )
		{
			TFSectionedListPanel *pPlayerList = NULL;
			int nTeam = g_PR->GetTeam( playerIndex );
			switch ( nTeam )
			{
			case TF_TEAM_BLUE:
				pPlayerList = m_pPlayerListBlue;
				break;
			case TF_TEAM_RED:
				pPlayerList = m_pPlayerListRed;
				break;
			}
			if ( null == pPlayerList )
				continue;

			KeyValues *pKeyValues = new KeyValues( "data" );
			pKeyValues->SetInt( "playerIndex", playerIndex );

			// this is just a placeholder in the sectioned list panel
			pKeyValues->SetInt( "medal", 0 ); 

			pKeyValues->SetString( "name", g_TF_PR->GetPlayerName( playerIndex ) );
			pKeyValues->SetInt( "score", g_TF_PR->GetTotalScore( playerIndex ) );

			int iClass = g_TF_PR->GetPlayerClass( playerIndex );
			if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS )
			{
				pKeyValues->SetInt( "class", tf_scoreboard_alt_class_icons.GetBool() ? m_iImageClassAlt[iClass] : m_iImageClass[iClass] );
			}
			else
			{
				pKeyValues->SetInt( "class", 0 );
			}

			pKeyValues->SetInt( "kills", g_TF_PR->GetPlayerScore( playerIndex ) );
			pKeyValues->SetInt( "damage", g_TF_PR->GetDamage( playerIndex ) );
			pKeyValues->SetInt( "healing", g_TF_PR->GetHealing( playerIndex ) );

			int nSupport = g_TF_PR->GetDamageAssist( playerIndex ) +
				g_TF_PR->GetHealingAssist( playerIndex ) +
				g_TF_PR->GetDamageBlocked( playerIndex ) +
				( g_TF_PR->GetBonusPoints( playerIndex ) * 25 );
			pKeyValues->SetInt( "support", nSupport );

			matchsummary_columns_t eParticleColumn = MS_COLUMN_INVALID;
			StatMedal_t eParticleMedal = StatMedal_None;

			if ( m_iCurrentState == MS_STATE_GOLD_MEDALS )
			{
				// we can add the bronze and silver since we've already processed those
				InternalAddMedalKeyValues( playerIndex, StatMedal_Bronze, pKeyValues );
				InternalAddMedalKeyValues( playerIndex, StatMedal_Silver, pKeyValues );
				eParticleColumn = InternalAddMedalKeyValues( playerIndex, StatMedal_Gold, pKeyValues, m_nMedalsRevealed );
				eParticleMedal = StatMedal_Gold;
			}
			else if ( m_iCurrentState == MS_STATE_SILVER_MEDALS )
			{
				// we can add the bronze since we've already processed those
				InternalAddMedalKeyValues( playerIndex, StatMedal_Bronze, pKeyValues );
				eParticleColumn = InternalAddMedalKeyValues( playerIndex, StatMedal_Silver, pKeyValues, m_nMedalsRevealed );
				eParticleMedal = StatMedal_Silver;
			}
			else if ( m_iCurrentState == MS_STATE_BRONZE_MEDALS )
			{
				eParticleColumn = InternalAddMedalKeyValues( playerIndex, StatMedal_Bronze, pKeyValues, m_nMedalsRevealed );
				eParticleMedal = StatMedal_Bronze;
			}

			UpdatePlayerAvatar( playerIndex, pKeyValues );

			int itemID = pPlayerList->AddItem( 0, pKeyValues );
			pPlayerList->SetItemFgColor( itemID, g_PR->GetTeamColor( nTeam ) );
			// Green background for rematch folks
			pPlayerList->SetItemBgColor( itemID, Color( 80, 80, 80, 80 ) );
			pPlayerList->SetItemBgHorizFillInset( itemID, pPlayerList->m_iHorizFillInset );
			pPlayerList->SetItemFont( itemID, m_hFont );

			// This highlights the local player in grey
			if ( playerIndex == GetLocalPlayerIndex() )
			{
				pPlayerList->SetSelectedItem( itemID );
			}

			// loop through and setup our medal color overrides
			KeyValues *pKey = pKeyValues->FindKey( "score_medal" );
			if ( pKey && pKey->GetInt() )
			{
				int nMedal = pKey->GetInt();
				pPlayerList->SetColorOverrideForCell( 0, itemID, MS_COLUMN_SCORE, ( nMedal == (int)StatMedal_Bronze ) ? m_clrBronzeMedal : ( nMedal == (int)StatMedal_Silver ) ?  m_clrSilverMedal : m_clrGoldMedal );
			}
			pKey = pKeyValues->FindKey( "kills_medal" );
			if ( pKey && pKey->GetInt() )
			{
				int nMedal = pKey->GetInt();
				pPlayerList->SetColorOverrideForCell( 0, itemID, MS_COLUMN_KILLS, ( nMedal == (int)StatMedal_Bronze ) ? m_clrBronzeMedal : ( nMedal == (int)StatMedal_Silver ) ? m_clrSilverMedal : m_clrGoldMedal );
			}
			pKey = pKeyValues->FindKey( "damage_medal" );
			if ( pKey && pKey->GetInt() )
			{
				int nMedal = pKey->GetInt();
				pPlayerList->SetColorOverrideForCell( 0, itemID, MS_COLUMN_DAMAGE, ( nMedal == (int)StatMedal_Bronze ) ? m_clrBronzeMedal : ( nMedal == (int)StatMedal_Silver ) ? m_clrSilverMedal : m_clrGoldMedal );
			}
			pKey = pKeyValues->FindKey( "healing_medal" );
			if ( pKey && pKey->GetInt() )
			{
				int nMedal = pKey->GetInt();
				pPlayerList->SetColorOverrideForCell( 0, itemID, MS_COLUMN_HEALING, ( nMedal == (int)StatMedal_Bronze ) ? m_clrBronzeMedal : ( nMedal == (int)StatMedal_Silver ) ? m_clrSilverMedal : m_clrGoldMedal );
			}
			pKey = pKeyValues->FindKey( "support_medal" );
			if ( pKey && pKey->GetInt() )
			{
				int nMedal = pKey->GetInt();
				pPlayerList->SetColorOverrideForCell( 0, itemID, MS_COLUMN_SUPPORT, ( nMedal == (int)StatMedal_Bronze ) ? m_clrBronzeMedal : ( nMedal == (int)StatMedal_Silver ) ? m_clrSilverMedal : m_clrGoldMedal );
			}

			pKeyValues->deleteThis();

			if ( ( eParticleColumn > MS_COLUMN_INVALID ) && ( eParticleMedal > StatMedal_None ) )
			{
				int nPanelXPos, nPanelYPos, nPanelWide, nPanelTall;
				pPlayerList->GetMaxCellBounds( itemID, eParticleColumn, nPanelXPos, nPanelYPos, nPanelWide, nPanelTall );

				FireMedalEffects( pPlayerList, nPanelXPos, nPanelYPos, nPanelWide, nPanelTall, eParticleMedal );
			}
		}
	}

	m_pPlayerListRed->SetSectionFgColor( 0, g_PR->GetTeamColor( TF_TEAM_RED ) );
	m_pPlayerListBlue->SetSectionFgColor( 0, g_PR->GetTeamColor( TF_TEAM_BLUE ) );

	// force the lists to PerformLayout() now so we can update our rank images after we return
	m_pPlayerListRed->InvalidateLayout( true );
	m_pPlayerListBlue->InvalidateLayout( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::FireMedalEffects( Panel *pPanel, int nPanelXPos, int nPanelYPos, int nPanelWide, int nPanelTall, StatMedal_t eParticleMedal )
{
	if ( !pPanel )
		return;

	int nPanelCenterX = nPanelXPos + ( nPanelWide / 2 );
	int nPanelCenterY = nPanelYPos + ( nPanelTall / 2 );

	int iItemAbsX, iItemAbsY;
	vgui::ipanel()->GetAbsPos( pPanel->GetParent()->GetVPanel(), iItemAbsX, iItemAbsY );

	int x = iItemAbsX + nPanelCenterX;
	int y = iItemAbsY + nPanelCenterY;

	const char *pszSoundEffect = "MatchMaking.None";
	const char *pszParticleEffect = "mvm_loot_smoke";
	if ( eParticleMedal == StatMedal_Bronze )
	{
		pszSoundEffect = "MatchMaking.Bronze";
		pszParticleEffect = "mvm_loot_explosion";
	}
	else if ( eParticleMedal == StatMedal_Silver )
	{
		pszSoundEffect = "MatchMaking.Silver";
		pszParticleEffect = "mvm_loot_explosion";
	}
	else if ( eParticleMedal == StatMedal_Gold )
	{
		pszSoundEffect = "MatchMaking.Gold";
		pszParticleEffect = "mvm_pow_gold_seq_firework_mid";
	}

	m_pParticlePanel->FireParticleEffect( pszParticleEffect, x, y, 0.2f, false );

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->EmitSound( pszSoundEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::UpdateBadgePanels( CUtlVector<CTFBadgePanel*> &pBadgePanels, TFSectionedListPanel *pPlayerList )
{
	if ( !TFGameRules() )
		return;

	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
	if ( pMatchDesc && pMatchDesc->m_pProgressionDesc )
	{
		if ( pPlayerList )
		{
			int iNumPanels = 0;
			int parentTall = pPlayerList->GetTall();
			CTFBadgePanel *pPanel = NULL;

			for ( int i = 0; i < pPlayerList->GetItemCount(); i++ )
			{
				KeyValues *pKeyValues = pPlayerList->GetItemData( i );
				if ( !pKeyValues )
					continue;

				const CSteamID steamID = GetSteamIDForPlayerIndex( pKeyValues->GetInt( "playerIndex" ) );
				if ( steamID.IsValid() )
				{
					if ( iNumPanels >= pBadgePanels.Count() )
					{
						pPanel = new CTFBadgePanel( m_pMainStatsContainer, "BadgePanel" );
						pPanel->MakeReadyForUse();
						pPanel->SetVisible( true );
						pPanel->SetZPos( 9999 );
						pBadgePanels.AddToTail( pPanel );
					}
					else
					{
						pPanel = pBadgePanels[iNumPanels];
					}

					int x, y, wide, tall;
					pPlayerList->GetMaxCellBounds( i, 0, x, y, wide, tall );

					if ( y + tall > parentTall )
						continue;

					if ( !pPanel->IsVisible() )
					{
						pPanel->SetVisible( true );
					}

					int xParent = 0, yParent = 0;
					if ( pPlayerList->GetParent() )
					{
						pPlayerList->GetParent()->GetPos( xParent, yParent );
					}

					int xGrandParent = 0, yGrandParent = 0;
					if ( pPlayerList->GetParent()->GetParent() )
					{
						pPlayerList->GetParent()->GetParent()->GetPos( xGrandParent, yGrandParent );
					}

					int nPanelXPos, nPanelYPos, nPanelWide, nPanelTall;
					pPanel->GetBounds( nPanelXPos, nPanelYPos, nPanelWide, nPanelTall );

					if ( ( nPanelXPos != xGrandParent + xParent + x )
						|| ( nPanelYPos != yGrandParent + yParent + y )
						|| ( nPanelWide != wide )
						|| ( nPanelTall != tall ) )
					{
						pPanel->SetBounds( xGrandParent + xParent + x, yGrandParent + yParent + y, wide, tall );
						pPanel->InvalidateLayout( true, true );
					}

					pPanel->SetupBadge( pMatchDesc, steamID );
					iNumPanels++;
				}
			}

			// hide any unused images
			for ( int i = iNumPanels; i < pBadgePanels.Count(); i++ )
			{
				if ( pBadgePanels[i]->IsVisible() )
				{
					pBadgePanels[i]->SetVisible( false );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	if ( !g_PR )
		return;

	uint32 iAccountID = g_PR->GetAccountID( playerIndex );
	if ( iAccountID > 0 )
	{
		// Update their avatar
		if ( kv && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
		{
			CSteamID steamIDForPlayer( iAccountID, 1, GetUniverse(), k_EAccountTypeIndividual );

			// See if we already have that avatar in our list
			int iMapIndex = m_mapAvatarsToImageList.Find( steamIDForPlayer );
			int iImageIndex;
			if ( iMapIndex == m_mapAvatarsToImageList.InvalidIndex() )
			{
				CAvatarImage *pImage = new CAvatarImage();
				pImage->SetAvatarSteamID( steamIDForPlayer );
				pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
				iImageIndex = m_pImageList->AddImage( pImage );

				m_mapAvatarsToImageList.Insert( steamIDForPlayer, iImageIndex );
			}
			else
			{
				iImageIndex = m_mapAvatarsToImageList[iMapIndex];
			}

			kv->SetInt( "avatar", iImageIndex );

			CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( iImageIndex );
			pAvIm->UpdateFriendStatus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CTFMatchSummary::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( FStrEq( type, "competitive_victory" ) )
	{
		Q_memset( m_SkillRatings, 0, sizeof( m_SkillRatings ) );
		Leaderboards_LadderRefresh();
	}
	else if ( FStrEq( type, "competitive_stats_update" ) )
	{
		int iIndex = event->GetInt( "index" );
		Assert( iIndex > 0 && iIndex <= MAX_PLAYERS );
		if ( iIndex > 0 && iIndex <= MAX_PLAYERS )
		{
			m_SkillRatings[iIndex].nScoreRank = event->GetInt( "score_rank" );		// Medal for Score (Gold, Silver, Bronze, or nothing)		
			m_SkillRatings[iIndex].nKillsRank = event->GetInt( "kills_rank" );		// Medal for Kills
			m_SkillRatings[iIndex].nDamageRank = event->GetInt( "damage_rank" );	// Medal for Damage
			m_SkillRatings[iIndex].nHealingRank = event->GetInt( "healing_rank" );	// Medal for Healing
			m_SkillRatings[iIndex].nSupportRank = event->GetInt( "support_rank" );	// Medal for Rank

			RecalculateMedalCounts();
		}
	}
	else if ( FStrEq( type, "player_abandoned_match" ) )
	{
		m_bPlayerAbandoned = true;
	}
	else if ( FStrEq( type, "client_disconnect" ) )
	{
		m_bPlayerAbandoned = false;
	}
	else if ( FStrEq( type, "show_match_summary" ) )
	{
		SetVisible( true );

		if ( m_pPlayerListRedParent->GetParent() )
		{
			int nRedBadgeOffset = m_pPlayerListRedParent->GetParent()->GetXPos();
			FOR_EACH_VEC( m_pRedBadgePanels, i )
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedBadgePanels[i], "xpos", m_pRedBadgePanels[i]->GetXPos() - nRedBadgeOffset, 0.25, 0.25, vgui::AnimationController::INTERPOLATOR_ACCEL );
			}
		}

		if ( m_pPlayerListBlueParent->GetParent() )
		{
			int nBlueBadgeOffset = m_pPlayerListBlueParent->GetParent()->GetXPos();
			FOR_EACH_VEC( m_pBlueBadgePanels, i )
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueBadgePanels[i], "xpos", m_pBlueBadgePanels[i]->GetXPos() - nBlueBadgeOffset, 0.25, 0.25, vgui::AnimationController::INTERPOLATOR_ACCEL );
			}
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pTeamScoresPanel, "HudMatchSummary_SlideInPanels", false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::InternalUpdateMedalCountForType( int iTeam, StatMedal_t eMedal )
{
	switch ( eMedal )
	{
	case StatMedal_Bronze:
		if ( iTeam == TF_TEAM_RED )
		{
			m_nMedalsToAward_Bronze_Red++;
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			m_nMedalsToAward_Bronze_Blue++;
		}
		break;
	case StatMedal_Silver:
		if ( iTeam == TF_TEAM_RED )
		{
			m_nMedalsToAward_Silver_Red++;
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			m_nMedalsToAward_Silver_Blue++;
		}
		break;
	case StatMedal_Gold:
		if ( iTeam == TF_TEAM_RED )
		{
			m_nMedalsToAward_Gold_Red++;
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			m_nMedalsToAward_Gold_Blue++;
		}
		break;
	case StatMedal_None:
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::RecalculateMedalCounts()
{
	m_nMedalsToAward_Bronze_Blue = 0;
	m_nMedalsToAward_Silver_Blue = 0;
	m_nMedalsToAward_Gold_Blue = 0;
	m_nMedalsToAward_Bronze_Red = 0;
	m_nMedalsToAward_Silver_Red = 0;
	m_nMedalsToAward_Gold_Red = 0;

	if ( !g_PR )
		return;

	for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( g_PR->IsConnected( playerIndex ) || g_PR->IsValid( playerIndex ) )
		{
			int nTeam = g_PR->GetTeam( playerIndex );
			if ( nTeam >= FIRST_GAME_TEAM )
			{
				InternalUpdateMedalCountForType( nTeam, (StatMedal_t)( m_SkillRatings[playerIndex].nScoreRank ) );
				InternalUpdateMedalCountForType( nTeam, (StatMedal_t)( m_SkillRatings[playerIndex].nKillsRank ) );
				InternalUpdateMedalCountForType( nTeam, (StatMedal_t)( m_SkillRatings[playerIndex].nDamageRank ) );
				InternalUpdateMedalCountForType( nTeam, (StatMedal_t)( m_SkillRatings[playerIndex].nHealingRank ) );
				InternalUpdateMedalCountForType( nTeam, (StatMedal_t)( m_SkillRatings[playerIndex].nSupportRank ) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::OnTick()
{
	BaseClass::OnTick();

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !IsVisible() || !pLocalPlayer )
		return;

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
	if ( !pMatchDesc )
		return;

	if ( pMatchDesc->BAllowDrawingAtMatchHistory()
	     && m_pDrawingPanel 
	     && ( m_flDrawingPanelTime > 0 ) 
	     && ( m_flDrawingPanelTime < gpGlobals->curtime ) )
	{
		m_pDrawingPanel->SetVisible( true );
		m_pDrawingPanel->SetKeyBoardInputEnabled( false );
		m_flDrawingPanelTime = -1.f;

		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Announcer.SummaryScreenWinners" );
		}
	}

	int nMedalsToAward_Bronze_Total = m_nMedalsToAward_Bronze_Blue + m_nMedalsToAward_Bronze_Red;
	int nMedalsToAward_Silver_Total = m_nMedalsToAward_Silver_Blue + m_nMedalsToAward_Silver_Red;
	int nMedalsToAward_Gold_Total = m_nMedalsToAward_Gold_Blue + m_nMedalsToAward_Gold_Red;

	bool bShowPerformanceMedals = ShowPerformanceMedals();
	bool bMapHasMatchSummaryStage = ( TFGameRules() && TFGameRules()->MapHasMatchSummaryStage() );

	
	bool bUseMatchSummaryStage = ( pMatchDesc && pMatchDesc->BUseMatchSummaryStage() );

	switch ( m_iCurrentState )
	{
	case MS_STATE_INITIAL:
	{
		bool bUseStage = ( bMapHasMatchSummaryStage && bUseMatchSummaryStage );

		if ( GTFGCClientSystem()->GetSurveyRequest().has_match_id() )
		{
			Panel* pSurveyPanel = CreateSurveyQuestionPanel( this, GTFGCClientSystem()->GetSurveyRequest() );
			pSurveyPanel->MakePopup();
		}

		m_iCurrentState = MS_STATE_DRAWING;
		m_flNextActionTime = bUseStage ? gpGlobals->curtime + MS_STATE_TRANSITION_TO_STATS : gpGlobals->curtime + 2.f;
		m_bXPShown = false;

		if ( !bUseStage )
		{
			// if we're not using the stage we'll just show the doors and then skip to stats with no drawing
			if ( m_pDrawingPanel )
			{
				m_pDrawingPanel->SetVisible( false );
			}
		}
		break;
	}
	case MS_STATE_DRAWING:
		{
			if ( gpGlobals->curtime > m_flNextActionTime )
			{
				if ( m_pDrawingPanel )
				{
					m_pDrawingPanel->SetVisible( false );
				}

				if ( m_pStatsBgPanel )
				{
					m_pStatsBgPanel->SetVisible( true );
				}

				if ( m_pStatsLabelPanel )
				{
					m_pStatsLabelPanel->SetVisible( true );
				}

				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pStatsLabelPanel, "ypos", m_bLargeMatchGroup ? m_iAnimStatsLabelPanel12v12YPos : m_iAnimStatsLabelPanel6v6YPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueMedalsPanel, "ypos", m_iAnimBlueMedalsYPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedMedalsPanel, "ypos", m_iAnimRedMedalsYPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueTeamName, "ypos", m_bLargeMatchGroup ? m_iAnimBlueTeamLabel12v12YPos : m_iAnimBlueTeamLabel6v6YPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamName, "ypos", m_bLargeMatchGroup ? m_iAnimRedTeamLabel12v12YPos : m_iAnimRedTeamLabel6v6YPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pPlayerListBlueParent, "wide", m_iAnimBluePlayerListParent, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueTeamScore, "wide", m_iAnimBlueTeamScore, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueTeamScoreDropshadow, "wide", m_iAnimBlueTeamScoreDropshadow, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueTeamScoreBG, "wide", m_iAnimBlueTeamScoreBG, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBluePlayerListBG, "wide",  m_iAnimBluePlayerListBG, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamScore, "wide", m_iAnimRedTeamScoreWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamScore, "xpos", m_iAnimRedTeamScoreXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamScoreDropshadow, "wide", m_iAnimRedTeamScoreDropshadowWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamScoreDropshadow, "xpos", m_iAnimRedTeamScoreDropshadowXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamScoreBG, "wide", m_iAnimRedTeamScoreBGWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamScoreBG, "xpos", m_iAnimRedTeamScoreBGXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pPlayerListRedParent, "wide", m_iAnimRedPlayerListParentWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pPlayerListRedParent, "xpos", m_iAnimRedPlayerListParentXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedPlayerListBG, "wide", m_iAnimRedPlayerListBGWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedPlayerListBG, "xpos", m_iAnimRedPlayerListBGXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueTeamWinner, "wide", m_iAnimBlueTeamScore, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBlueTeamWinnerDropshadow, "wide", m_iAnimBlueTeamScoreDropshadow, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamWinner, "wide", m_iAnimRedTeamScoreWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamWinner, "xpos", m_iAnimRedTeamScoreXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamWinnerDropshadow, "wide", m_iAnimRedTeamScoreDropshadowWide, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedTeamWinnerDropshadow, "xpos", m_iAnimRedTeamScoreDropshadowXPos, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );

				int nRedBadgeOffset = m_pPlayerListRedParent->GetXPos() - m_iAnimRedPlayerListParentXPos;
				FOR_EACH_VEC( m_pRedBadgePanels, i )
				{
					g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pRedBadgePanels[i], "xpos", m_pRedBadgePanels[i]->GetXPos() - nRedBadgeOffset, 0.0, 0.1, vgui::AnimationController::INTERPOLATOR_ACCEL );

				}

				float flDelay = MS_STATE_TRANSITION_TO_MEDALS;
					
				if ( pLocalPlayer )
				{
					if ( bShowPerformanceMedals )
					{
						const char *pszEntryName = UTIL_GetRandomSoundFromEntry( "Announcer.CompSummaryScreenOutlierQuestion" );
						if ( pszEntryName && pszEntryName[0] )
						{
							flDelay = enginesound->GetSoundDuration( pszEntryName );
							pLocalPlayer->EmitSound( pszEntryName );
						}
					}
					pLocalPlayer->EmitSound( "MatchMaking.ScoreboardPanelSlide" );
				}

				m_iCurrentState = MS_STATE_STATS;
				m_flNextActionTime = gpGlobals->curtime + flDelay;

				m_pRedMedalsPanel->SetVisible( bShowPerformanceMedals );
				m_pBlueMedalsPanel->SetVisible( bShowPerformanceMedals );

				m_pStatsAndMedals->SetText( bShowPerformanceMedals ? g_pVGuiLocalize->Find( "#TF_CompSummary_StatsAndMedals" ) : L"" );
				m_pStatsAndMedalsShadow->SetText( bShowPerformanceMedals ? g_pVGuiLocalize->Find( "#TF_CompSummary_StatsAndMedals" ) : L"" );
			}
			break;
		}
	case MS_STATE_STATS:
		{
			if ( gpGlobals->curtime > m_flNextActionTime )
			{
				m_iCurrentState = bShowPerformanceMedals ? MS_STATE_BRONZE_MEDALS : MS_STATE_FINAL;
				m_nMedalsRevealed = 0;
				m_flNextActionTime = -1;
			}
			break;
		}
	case MS_STATE_BRONZE_MEDALS:
		{
			if ( gpGlobals->curtime > m_flNextActionTime )
			{
				if ( !m_bBlueBronzeValueRevealed && !m_bRedBronzeValueRevealed )
				{
					m_pBlueMedalsPanel->SetDialogVariable( "blueteammedals_bronze", m_nMedalsToAward_Bronze_Blue );
					m_bBlueBronzeValueRevealed = true;
					Panel *pChild = m_pBlueMedalsPanel->FindChildByName( "BlueBronzeMedalValue" );
					if ( pChild )
					{
						int nXPos, nYPos, nWide, nTall;
						pChild->GetBounds( nXPos, nYPos, nWide, nTall );
						FireMedalEffects( pChild, nXPos, nYPos, nWide, nTall, ( m_nMedalsToAward_Bronze_Blue > 0 ) ? StatMedal_Bronze : StatMedal_None );
					}

					m_pRedMedalsPanel->SetDialogVariable( "redteammedals_bronze", m_nMedalsToAward_Bronze_Red );
					m_bRedBronzeValueRevealed = true;
					pChild = m_pRedMedalsPanel->FindChildByName( "RedBronzeMedalValue" );
					if ( pChild )
					{
						int nXPos, nYPos, nWide, nTall;
						pChild->GetBounds( nXPos, nYPos, nWide, nTall );
						FireMedalEffects( pChild, nXPos, nYPos, nWide, nTall, ( m_nMedalsToAward_Bronze_Red > 0 ) ? StatMedal_Bronze : StatMedal_None );
					}

					m_flNextActionTime = gpGlobals->curtime + MS_STATE_TIME_BETWEEN_MEDALS_CATEGORIES;
				}
				else
				{
					if ( ( nMedalsToAward_Bronze_Total > 0 ) && ( m_nMedalsRevealed < nMedalsToAward_Bronze_Total ) )
					{
						Update();
						m_nMedalsRevealed++;
						m_flNextActionTime = gpGlobals->curtime + MS_STATE_TIME_BETWEEN_MEDALS;
					}
					else
					{
						m_iCurrentState = MS_STATE_SILVER_MEDALS;
						m_nMedalsRevealed = 0;
						m_flNextActionTime = MS_STATE_TIME_BETWEEN_MEDALS_CATEGORIES;
					}
				}
			}
			break;
		}
	case MS_STATE_SILVER_MEDALS:
		{
			if ( gpGlobals->curtime > m_flNextActionTime )
			{
				if ( !m_bBlueSilverValueRevealed && !m_bRedSilverValueRevealed )
				{
					m_pBlueMedalsPanel->SetDialogVariable( "blueteammedals_silver", m_nMedalsToAward_Silver_Blue );
					m_bBlueSilverValueRevealed = true;
					Panel *pChild = m_pBlueMedalsPanel->FindChildByName( "BlueSilverMedalValue" );
					if ( pChild )
					{
						int nXPos, nYPos, nWide, nTall;
						pChild->GetBounds( nXPos, nYPos, nWide, nTall );
						FireMedalEffects( pChild, nXPos, nYPos, nWide, nTall, ( m_nMedalsToAward_Silver_Blue > 0 ) ? StatMedal_Silver : StatMedal_None );
					}

					m_pRedMedalsPanel->SetDialogVariable( "redteammedals_silver", m_nMedalsToAward_Silver_Red );
					m_bRedSilverValueRevealed = true;
					pChild = m_pRedMedalsPanel->FindChildByName( "RedSilverMedalValue" );
					if ( pChild )
					{
						int nXPos, nYPos, nWide, nTall;
						pChild->GetBounds( nXPos, nYPos, nWide, nTall );
						FireMedalEffects( pChild, nXPos, nYPos, nWide, nTall, ( m_nMedalsToAward_Silver_Red > 0 ) ? StatMedal_Silver : StatMedal_None );
					}

					m_flNextActionTime = gpGlobals->curtime + MS_STATE_TIME_BETWEEN_MEDALS_CATEGORIES;
				}
				else
				{
					if ( ( nMedalsToAward_Silver_Total > 0 ) && ( m_nMedalsRevealed < nMedalsToAward_Silver_Total ) )
					{
						Update();
						m_nMedalsRevealed++;
						m_flNextActionTime = gpGlobals->curtime + MS_STATE_TIME_BETWEEN_MEDALS;
					}
					else
					{
						m_iCurrentState = MS_STATE_GOLD_MEDALS;
						m_nMedalsRevealed = 0;
						m_flNextActionTime = MS_STATE_TIME_BETWEEN_MEDALS_CATEGORIES;
					}
				}
			}
			break;
		}
	case MS_STATE_GOLD_MEDALS:
		{
			if ( gpGlobals->curtime > m_flNextActionTime )
			{
				if ( !m_bBlueGoldValueRevealed && !m_bRedGoldValueRevealed )
				{
					m_pBlueMedalsPanel->SetDialogVariable( "blueteammedals_gold", m_nMedalsToAward_Gold_Blue );
					m_bBlueGoldValueRevealed = true;
					Panel *pChild = m_pBlueMedalsPanel->FindChildByName( "BlueGoldMedalValue" );
					if ( pChild )
					{
						int nXPos, nYPos, nWide, nTall;
						pChild->GetBounds( nXPos, nYPos, nWide, nTall );
						FireMedalEffects( pChild, nXPos, nYPos, nWide, nTall, ( m_nMedalsToAward_Gold_Blue > 0 ) ? StatMedal_Gold : StatMedal_None );
					}

					m_pRedMedalsPanel->SetDialogVariable( "redteammedals_gold", m_nMedalsToAward_Gold_Red );
					m_bRedGoldValueRevealed = true;
					pChild = m_pRedMedalsPanel->FindChildByName( "RedGoldMedalValue" );
					if ( pChild )
					{
						int nXPos, nYPos, nWide, nTall;
						pChild->GetBounds( nXPos, nYPos, nWide, nTall );
						FireMedalEffects( pChild, nXPos, nYPos, nWide, nTall, ( m_nMedalsToAward_Gold_Red > 0 ) ? StatMedal_Gold : StatMedal_None );
					}

					m_flNextActionTime = gpGlobals->curtime + MS_STATE_TIME_BETWEEN_MEDALS_CATEGORIES;
				}
				else
				{
					if ( ( nMedalsToAward_Gold_Total > 0 ) && ( m_nMedalsRevealed < nMedalsToAward_Gold_Total ) )
					{
						Update();
						m_nMedalsRevealed++;
						m_flNextActionTime = gpGlobals->curtime + MS_STATE_TIME_BETWEEN_MEDALS;
					}
					else
					{
						m_iCurrentState = MS_STATE_FINAL;
						m_nMedalsRevealed = 0;
						m_flNextActionTime = -1;

						if ( pLocalPlayer )
						{
							const char *pszSoundScriptEntry = "Announcer.CompSummaryScreenOutlierNo"; 
							if ( nMedalsToAward_Bronze_Total || nMedalsToAward_Silver_Total || nMedalsToAward_Gold_Total )
							{
								pszSoundScriptEntry = "Announcer.CompSummaryScreenOutlierYes";
							}

							const char *pszSoundName = UTIL_GetRandomSoundFromEntry( pszSoundScriptEntry );
							m_flMedalSoundTime = gpGlobals->curtime + enginesound->GetSoundDuration( pszSoundName ) + 0.5f;
							pLocalPlayer->EmitSound( pszSoundName );

							IGameEvent *event = gameeventmanager->CreateEvent( "ds_screenshot" );
							if ( event )
							{
								event->SetFloat( "delay", 0.5f );
								gameeventmanager->FireEventClientSide( event );
							}
						}
					}
				}
			}
			break;
		}
	default:
	case MS_STATE_FINAL:
		{
			bool bMedalSoundTimeComplete = ( m_flMedalSoundTime > 0 ) && ( m_flMedalSoundTime < gpGlobals->curtime );

			if ( !m_bXPShown /*&& ( !bShowMedals || bMedalSoundTimeComplete ) */)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "CompetitiveGame_ShowPvPRankPanel", false );	
				m_bXPShown = true;
			}

			if ( bShowPerformanceMedals )
			{
				if ( bMedalSoundTimeComplete )
				{
					m_flMedalSoundTime = -1.f;
					int iLocalPlayerIndex = GetLocalPlayerIndex();

					if ( ( m_SkillRatings[iLocalPlayerIndex].nScoreRank != StatMedal_None ) ||
						 ( m_SkillRatings[iLocalPlayerIndex].nKillsRank != StatMedal_None ) ||
						 ( m_SkillRatings[iLocalPlayerIndex].nDamageRank != StatMedal_None ) ||
						 ( m_SkillRatings[iLocalPlayerIndex].nHealingRank != StatMedal_None ) ||
						 ( m_SkillRatings[iLocalPlayerIndex].nSupportRank != StatMedal_None ) )
					{
						if ( pLocalPlayer )
						{
							int iClass = RandomInt( TF_CLASS_SCOUT, TF_CLASS_ENGINEER );
							if ( pLocalPlayer->GetPlayerClass() && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() > TF_CLASS_UNDEFINED ) )
							{
								iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();
							}

							pLocalPlayer->EmitSound( VarArgs( "%s.CompSummaryScreenOutlier", g_aPlayerClassNames_NonLocalized[iClass] ) );
						}
					}
				}
			}

			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMatchSummary::LevelInit( void )
{
	SetVisible( false );
}

void CTFMatchSummary::LevelShutdown( void )
{
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMatchSummary::ShowPerformanceMedals( void )
{
	bool bDistributePerformanceMedals = false;

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		bDistributePerformanceMedals = pMatchDesc->BDistributePerformanceMedals();
	}

	return ( bDistributePerformanceMedals && !m_bPlayerAbandoned );
}
