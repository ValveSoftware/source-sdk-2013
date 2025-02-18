//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>

#include "vgui_avatarimage.h"
#include "tf_hud_match_status.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "vgui_controls/ScalableImagePanel.h"
#include "tf_time_panel.h"
#include "c_team_objectiveresource.h"
#include "game_controls/spectatorgui.h"
#include "c_tf_playerresource.h"
#include "tf_gc_client.h"
#include "tf_match_description.h"
#include "tf_hud_tournament.h"
#include "tf_classmenu.h"
#include "tf_rating_data.h"

extern ConVar mp_winlimit;
extern ConVar mp_tournament_stopwatch;

ConVar tf_use_match_hud( "tf_use_match_hud", "1", FCVAR_ARCHIVE );

using namespace vgui;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
bool IsTakingAFreezecamScreenshot();

//-----------------------------------------------------------------------------
// Purpose: Use the new match HUD or the old?  Right now, Comp is the key
//-----------------------------------------------------------------------------
bool ShouldUseMatchHUD()
{
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		return false;

	return tf_use_match_hud.GetBool();

}

const int g_nMaxSupportedRounds = 5;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRoundCounterPanel::CRoundCounterPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
	, m_pRoundIndicatorKVs( NULL )
	, m_pRoundWinIndicatorRedKV( NULL )
	, m_pRoundWinIndicatorBlueKV( NULL )
	, m_bCountDirty( false )
{
	ListenForGameEvent( "winlimit_changed" );
	ListenForGameEvent( "winpanel_show_scores" );
	ListenForGameEvent( "stop_watch_changed" );
	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRoundCounterPanel::~CRoundCounterPanel()
{
	if ( m_pRoundIndicatorKVs )
		m_pRoundIndicatorKVs->deleteThis();

	if ( m_pRoundWinIndicatorRedKV )
		m_pRoundWinIndicatorRedKV->deleteThis();

	if ( m_pRoundWinIndicatorBlueKV )
		m_pRoundWinIndicatorBlueKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRoundCounterPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudRoundCounter.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Put a copy of the specified keys in block pszKeyName from pKVIn
//			into pKV
//-----------------------------------------------------------------------------
void LoadKeyValues( KeyValues** pKV, KeyValues* pKVIn, const char* pszKeyName )
{
	if ( (*pKV) )
		(*pKV)->deleteThis();

	(*pKV) = pKVIn->FindKey( pszKeyName );
	if ((*pKV))
	{
		(*pKV) = (*pKV)->MakeCopy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRoundCounterPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	LoadKeyValues( &m_pRoundIndicatorKVs, inResourceData, "RoundIndicatorPanel_kv" );
	LoadKeyValues( &m_pRoundWinIndicatorRedKV, inResourceData, "RoundWinPanelRed_kv" );
	LoadKeyValues( &m_pRoundWinIndicatorBlueKV, inResourceData, "RoundWinPanelBlue_kv" );

	CreateRoundPanels( m_vecBlueRoundIndicators, "RoundIndicator", m_pRoundIndicatorKVs );
	CreateRoundPanels( m_vecRedRoundIndicators, "RoundIndicator", m_pRoundIndicatorKVs );
	CreateRoundPanels( m_vecBlueWinIndicators, "WinIndicatorBlue", m_pRoundWinIndicatorBlueKV );
	CreateRoundPanels( m_vecRedWinIndicators, "WinIndicatorRed", m_pRoundWinIndicatorRedKV );
}

//-----------------------------------------------------------------------------
// Purpose: Ensure there are the correct number of image panels.  If not, create
//			them and apply the passed-in settings
//-----------------------------------------------------------------------------
void CRoundCounterPanel::CreateRoundPanels( ImageVector& vecImages, const char* pszName, KeyValues* pKVSettings )
{
	int nMaxRounds = g_nMaxSupportedRounds;

	if ( vecImages.Count() != nMaxRounds )
	{
		FOR_EACH_VEC( vecImages, i )
		{
			vecImages[ i ]->MarkForDeletion();
		}

		vecImages.Purge();
		
		if ( nMaxRounds > 0 )
		{
			while ( nMaxRounds-- )
			{
				vecImages.AddToTail(new ImagePanel(this, pszName));
			}
		}
	}

	if ( pKVSettings )
	{
		FOR_EACH_VEC(vecImages, i)
		{
			vecImages[i]->ApplySettings( pKVSettings );
		}
	}
}

extern ConVar tf_attack_defend_map;
//-----------------------------------------------------------------------------
// Purpose: Loop through and conditionally set visible some panels
//-----------------------------------------------------------------------------
void VisibleCondition( CRoundCounterPanel::ImageVector& vecImages, int iMax )
{
	bool bInStopWatch = tf_attack_defend_map.GetBool();

	FOR_EACH_VEC( vecImages, i )
	{
		vecImages[i]->SetVisible( i < iMax && !bInStopWatch );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Position all of the round panels and resize the background blue/red
//-----------------------------------------------------------------------------
void CRoundCounterPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( !TFGameRules() || !ShouldUseMatchHUD() )
		return;

	C_TFTeam* pTeams[ TF_TEAM_COUNT ];
	pTeams[ TF_TEAM_RED ] = GetGlobalTFTeam( TF_TEAM_RED );
	pTeams[ TF_TEAM_BLUE ] = GetGlobalTFTeam( TF_TEAM_BLUE );

	if ( !pTeams[ TF_TEAM_RED ] || !pTeams[ TF_TEAM_BLUE ] )
		return;

	// Layout the round indicators
	LayoutPanels( m_vecBlueRoundIndicators, EAlignment::ALIGN_WEST, (GetWide() / 2) - m_nIndicatorStartOffset, m_nIndicatorPanelStep );
	VisibleCondition( m_vecBlueRoundIndicators, mp_winlimit.GetInt() );
	LayoutPanels( m_vecRedRoundIndicators, EAlignment::ALIGN_EAST, (GetWide() / 2) + m_nIndicatorStartOffset, m_nIndicatorPanelStep );
	VisibleCondition( m_vecRedRoundIndicators, mp_winlimit.GetInt() );
	// Layout the win indicators
	LayoutPanels( m_vecBlueWinIndicators, EAlignment::ALIGN_WEST, (GetWide() / 2) - m_nIndicatorStartOffset, m_nIndicatorPanelStep );
	VisibleCondition( m_vecBlueWinIndicators, Min( mp_winlimit.GetInt(), pTeams[ TF_TEAM_BLUE ]->m_iScore ) );
	LayoutPanels( m_vecRedWinIndicators, EAlignment::ALIGN_EAST, (GetWide() / 2) + m_nIndicatorStartOffset, m_nIndicatorPanelStep );
	VisibleCondition( m_vecRedWinIndicators, Min( mp_winlimit.GetInt(), pTeams[ TF_TEAM_RED ]->m_iScore ) );
}

void CRoundCounterPanel::OnThink()
{
	if ( m_bCountDirty )
	{
		int nNumVisible = 0;
		FOR_EACH_VEC( m_vecBlueRoundIndicators, i )
		{
			if ( m_vecBlueRoundIndicators[i]->IsVisible() )
				++nNumVisible;
		}

		if ( nNumVisible != mp_winlimit.GetInt() )
		{
			InvalidateLayout();
			m_bCountDirty = false;
		}
	}
}

void CRoundCounterPanel::FireGameEvent(IGameEvent * event )
{
	if ( FStrEq( event->GetName(), "winlimit_changed" ) )	// Resize if the win limit changes
	{
		m_bCountDirty = true;
	}
	else if ( FStrEq( event->GetName(), "winpanel_show_scores" ) // Conditionally hide the win markers
		   || FStrEq( event->GetName(), "stop_watch_changed" )		// Match the timing of the win panel "Ding!" when the scores update
		   || FStrEq( event->GetName(), "teamplay_round_start" ) ) // Make sure we're accurate when the round starts in case the hud event didnt happen
	{
		InvalidateLayout( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Layout the round panels
//-----------------------------------------------------------------------------
void CRoundCounterPanel::LayoutPanels( ImageVector& vecImages, EAlignment eAlignment, int nStartPos, int nMaxWide )
{
	if ( !mp_winlimit.GetInt() )
		return;

	FOR_EACH_VEC( vecImages, i )
	{
		Panel* pPanel = vecImages[ i ];

		const int nXStartPos = eAlignment == ALIGN_EAST ? nStartPos : nStartPos;
		const int nStep = ( nMaxWide / mp_winlimit.GetInt() );
		const int nXOffset = nStep * i;
		// Step out the panels by the steph width
		int nXPos = eAlignment == ALIGN_EAST ? nXStartPos + nXOffset - ( pPanel->GetWide() / 2 ) + ( nStep / 2 )
											 : nXStartPos - nXOffset - ( pPanel->GetWide() / 2 ) - ( nStep / 2 );
		pPanel->SetPos( nXPos, pPanel->GetYPos() );
	}
}



DECLARE_HUDELEMENT( CTFHudMatchStatus );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudMatchStatus::CTFHudMatchStatus(const char *pElementName) 
	: CHudElement(pElementName)
	, BaseClass(NULL, "HudMatchStatus")
	, m_pTimePanel( NULL )
	, m_bUseMatchHUD( false )
	, m_eMatchGroupSettings( k_eTFMatchGroup_Invalid )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_MATCH_STATUS );

	m_pMatchStartModelPanel = new CModelPanel( this, "MatchDoors" );

	m_pRoundCounter = new CRoundCounterPanel( this, "RoundCounter" );
	m_pTimePanel = new CTFHudTimeStatus( this, "ObjectiveStatusTimePanel" );
	m_pRoundSignModel = new CModelPanel( this, "RoundSignModel" );
	m_pTeamStatus = new CTFTeamStatus( this, "TeamStatus" );

	m_pBlueTeamPanel = new vgui::EditablePanel( this, "BlueTeamPanel" );
	m_pPlayerListBlue = new vgui::SectionedListPanel( m_pBlueTeamPanel, "BluePlayerList" );
	m_pBlueLeaderAvatarImage = new CAvatarImagePanel( m_pBlueTeamPanel, "BlueLeaderAvatar" );
	m_pBlueLeaderAvatarBG = new EditablePanel( m_pBlueTeamPanel, "BlueLeaderAvatarBG" );
	m_pBlueTeamImage = new ImagePanel( m_pBlueTeamPanel, "BlueTeamImage" );
	m_pBlueTeamName = new CExLabel( m_pBlueTeamPanel, "BlueTeamLabel", "" );
	m_pRedTeamPanel = new vgui::EditablePanel( this, "RedTeamPanel" );
	m_pPlayerListRed = new vgui::SectionedListPanel( m_pRedTeamPanel, "RedPlayerList" );
	m_pRedLeaderAvatarImage = new CAvatarImagePanel( m_pRedTeamPanel, "RedLeaderAvatar" );
	m_pRedLeaderAvatarBG = new EditablePanel( m_pRedTeamPanel, "RedLeaderAvatarBG" );
	m_pRedTeamImage = new ImagePanel( m_pRedTeamPanel, "RedTeamImage" );
	m_pRedTeamName = new CExLabel( m_pRedTeamPanel, "RedTeamLabel", "" );

	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapAvatarsToImageList.RemoveAll();

	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "restart_timer_time" );
	ListenForGameEvent( "show_match_summary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudMatchStatus::~CTFHudMatchStatus()
{
	if ( NULL != m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::Reset()
{
	SetPanelsVisible();

	if ( m_pTimePanel )
	{
		m_pTimePanel->Reset();
	}

	if ( m_pTeamStatus )
	{
		m_pTeamStatus->Reset();
	}

	CHudElement::Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::SetPanelsVisible()
{
	m_pRoundCounter->SetVisible( ShouldUseMatchHUD() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	KeyValues *pConditions = NULL;
	if ( ShouldUseMatchHUD() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_match" );

		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GTFGCClientSystem()->GetLiveMatchGroup() );
		if ( pMatchDesc )
		{
			if ( pMatchDesc->GetMatchSize() > 12 )
			{
				AddSubKeyNamed( pConditions, "if_large" );
			}
		}
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudMatchStatus.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	if ( m_pImageList )
		delete m_pImageList;

	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();

	m_pPlayerListBlue->SetImageList( m_pImageList, false );
	m_pPlayerListRed->SetImageList( m_pImageList, false );

	InitPlayerList( m_pPlayerListBlue, TF_TEAM_BLUE );
	InitPlayerList( m_pPlayerListRed, TF_TEAM_RED );

	m_hPlayerListFont = pScheme->GetFont( "Default", true );

	UpdatePlayerList();
	UpdateTeamInfo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::PerformLayout()
{
	BaseClass::PerformLayout();

	SetPanelsVisible();
}

bool CTFHudMatchStatus::IsVisible( void )
{
	// Hide panel for freeze-cam screenshot?
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

bool CTFHudMatchStatus::ShouldDraw( void )
{
	// Force to draw during match summary so the doors show up.  This panel 
	// will try to hide itself if you're dead, but we want to ignore that
	// behavior and force us to draw.
	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		  return true;

	if ( gViewPortInterface->GetActivePanel() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::OnThink()
{
	if ( !TFGameRules() )
		return;

	bool bReload = false;
	bool bUseMatchHUD = ShouldUseMatchHUD();

	if ( bUseMatchHUD != m_bUseMatchHUD )
	{
		m_bUseMatchHUD = bUseMatchHUD;
		bReload = true;
	}

	ETFMatchGroup eCurrentGroup = TFGameRules()->GetCurrentMatchGroup();
	if ( eCurrentGroup != m_eMatchGroupSettings )
	{
		m_eMatchGroupSettings = eCurrentGroup;
		bReload = true;
	}

	if ( bReload )
	{
		InvalidateLayout( false, true );

		// The KOTH timers are their own hud element 
		CTFHudKothTimeStatus *pKothHUD = GET_HUDELEMENT( CTFHudKothTimeStatus );
		if ( pKothHUD )
		{
			pKothHUD->InvalidateLayout( false, true );
		}
	}

	// check for an active timer and turn the time panel on or off if we need to
	if ( m_pTimePanel )
	{
		// Don't draw in freezecam, or when the game's not running
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		bool bDisplayTimer = !( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM );

		if ( TeamplayRoundBasedRules()->IsInTournamentMode() && TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		{
			bDisplayTimer = false;
		}

		if ( bDisplayTimer )
		{
			// is the time panel still pointing at an active timer?
			int iCurrentTimer = m_pTimePanel->GetTimerIndex();
			CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iCurrentTimer ) );

			if ( pTimer && !pTimer->IsDormant() && !pTimer->IsDisabled() && pTimer->ShowInHud() )
			{
				// the current timer is fine, make sure the panel is visible
				bDisplayTimer = true;
			}
			else if ( ObjectiveResource() )
			{
				// check for a different timer
				int iActiveTimer = ObjectiveResource()->GetTimerToShowInHUD();

				pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( iActiveTimer ) );
				bDisplayTimer = ( iActiveTimer != 0 && pTimer && !pTimer->IsDormant() );
				m_pTimePanel->SetTimerIndex( iActiveTimer );
			}
		}

		if ( bDisplayTimer && !TFGameRules()->ShowMatchSummary() )
		{
			if ( !TFGameRules()->IsInKothMode() )
			{
				if ( !m_pTimePanel->IsVisible() )
				{
					m_pTimePanel->SetVisible( true );

					// If our spectator GUI is visible, invalidate its layout so that it moves the reinforcement label
					if ( g_pSpectatorGUI )
					{
						g_pSpectatorGUI->InvalidateLayout();
					}
				}
			}
			else
			{
				bool bVisible = TeamplayRoundBasedRules()->IsInWaitingForPlayers();

				if ( m_pTimePanel->IsVisible() != bVisible )
				{
					m_pTimePanel->SetVisible( bVisible );
	
					// If our spectator GUI is visible, invalidate its layout so that it moves the reinforcement label
					if ( g_pSpectatorGUI )
					{
						g_pSpectatorGUI->InvalidateLayout();
					}
				}
			}
		}
		else 
		{
			if ( m_pTimePanel->IsVisible() )
			{
				m_pTimePanel->SetVisible( false );
			}
		}
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::FireGameEvent( IGameEvent * event )
{
	if ( !ShouldUseMatchHUD() )
		return;

	if ( FStrEq("teamplay_round_start", event->GetName() ) )
	{
		// Drop the round sign right when the match starts on rounds > 1
		if ( TFGameRules()->GetRoundsPlayed() > 0 )
		{
			ShowRoundSign( TFGameRules()->GetRoundsPlayed() );
		}
	}
	else if ( FStrEq( "restart_timer_time", event->GetName() ) )
	{
		HandleCountdown( event->GetInt( "time" ) );
	}
	else if ( FStrEq( "show_match_summary", event->GetName() ) ) 
	{
		if ( m_pBlueTeamPanel )
		{
			m_pBlueTeamPanel->SetVisible( false );
		}

		if ( m_pRedTeamPanel )
		{
			m_pRedTeamPanel->SetVisible( false );
		}

		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		bool bForceDoors = false;
		if ( bForceDoors || ( pMatchDesc && pMatchDesc->BUsesPostRoundDoors() ) )
		{
			if ( TFGameRules() && TFGameRules()->MapHasMatchSummaryStage() && ( bForceDoors || pMatchDesc->BUseMatchSummaryStage() ) )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMatchStatus_ShowMatchWinDoors", false );
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMatchStatus_ShowMatchWinDoors_NoOpen", false );
			}
		}
	}
}

void CTFHudMatchStatus::HandleCountdown( int nTime )
{
	// Update the timer
	SetDialogVariable( "countdown", nTime );

	switch ( nTime )
	{
	case 2:
		// Drop the round sign with 2 seconds to go on the 1st round
		if ( TFGameRules()->GetRoundsPlayed() == 0 )
		{
			ShowRoundSign( TFGameRules()->GetRoundsPlayed() );
		}
		break;
	case 10:
		if ( TFGameRules()->GetRoundsPlayed() == 0 )
		{
			ShowMatchStartDoors();
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMatchStatus_ShowCountdown", false );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::ShowMatchStartDoors()
{
	if ( TFGameRules()->GetCurrentMatchGroup() == k_eTFMatchGroup_Invalid )
		return;

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );

	int nSkin = 0;
	int nSubModel = 0;
	if ( pMatchDesc->BGetRoundDoorParameters( nSkin, nSubModel ) )
	{
		UpdatePlayerList();
		UpdateTeamInfo();

		if ( m_pMatchStartModelPanel->m_hModel == NULL )
		{
			m_pMatchStartModelPanel->UpdateModel();
		}

		m_pMatchStartModelPanel->SetBodyGroup( "logos", nSubModel );
		m_pMatchStartModelPanel->UpdateModel();
		m_pMatchStartModelPanel->SetSkin( nSkin );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMatchStatus_ShowMatchStartDoors", false );

		bool bUsesStickyRanks = pMatchDesc->BUsesStickyRanks();
		SetControlVisible( "RankUpLabel", bUsesStickyRanks, true );
		SetControlVisible( "RankUpShadowLabel", bUsesStickyRanks, true );

		// For competitive games that use sticky ratings, we want to show a "You'll rank up if you win!" message
		// at the beginning of a match would trigger a rank up if the user wins.
		if ( bUsesStickyRanks )
		{
			auto pRating = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( SteamUser()->GetSteamID(), pMatchDesc->GetCurrentDisplayRating() );
			auto pRank = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( SteamUser()->GetSteamID(), pMatchDesc->GetCurrentDisplayRank() );
			bool bInPlacement = pMatchDesc->BLocalPlayerIsInPlacement();

			if ( bInPlacement && pMatchDesc->GetNumPlacementMatchesToGo( steamapicontext->SteamUser()->GetSteamID() ) == 1 )
			{
				// For exiting placement, put up a message that indicates so
				SetDialogVariable( "rank_possibility", g_pVGuiLocalize->Find( "#TF_MM_PlacementMatch" ) );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMatchStatus_ShowRankMatch", false );
			}
			else if ( !bInPlacement && pRank && pRating )
			{
				// Make sure their new rank is above their current rating, meaning they're trending
				// towards a rank-up and not a rank-down
				uint32 nRankForNewRating = pMatchDesc->m_pProgressionDesc->GetLevelForRating( pRating->GetRatingData().unRatingPrimary ).m_nDisplayLevel;
				bool bTrendingUp = nRankForNewRating > pRank->GetRatingData().unRatingPrimary;

				// So long as your secondary (games in this rank) is >= 4 (meaning this game will push you into the requisite 5)
				// and your tertiary (games since rank change) is >= 9 (meaning this game will push you into the requisite 10),
				// then you're on the threshold of a rank up.  You could have secondary 6 and tertiary 11, meaning you lost a couple
				// times while on the threshold, but the message is still valid.
				bool bOnRankUpThreshold = pRank->GetRatingData().unRatingSecondary >= (k_nLadder_MinGamesInThresholdToRank - 1) &&
					pRank->GetRatingData().unRatingTertiary >= (k_nLadder_MinGamesBetweenRankChanges - 1);

				if ( bTrendingUp && bOnRankUpThreshold )
				{
					SetDialogVariable( "rank_possibility", g_pVGuiLocalize->Find( "#TF_MM_RankUpMatch" ) );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMatchStatus_ShowRankMatch", false );
				}
			}
		}

		// Hide the class selection panel.  It sorts weird with the doors, and we dont have time to figure out why.
		gViewPortInterface->ShowPanel( PANEL_CLASS_RED, false );
		gViewPortInterface->ShowPanel( PANEL_CLASS_BLUE, false );

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( pMatchDesc->GetMatchStartSound() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show the round sign with the specified round number
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::ShowRoundSign( int nRoundNumber )
{
	if ( TFGameRules()->GetCurrentMatchGroup() == k_eTFMatchGroup_Invalid )
		return;

	if ( !m_pRoundSignModel || !m_pRoundSignModel->m_pModelInfo )
		return;

	Assert( TFGameRules()->GetRoundsPlayed() >= 0 && TFGameRules()->GetRoundsPlayed() <= 6 );

	int nSkin = 0;
	int nBodyGroup = 0;
	if ( GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() )->BGetRoundStartBannerParameters( nSkin, nBodyGroup ) )
	{
		if ( m_pRoundSignModel->m_hModel == NULL )
		{
			m_pRoundSignModel->UpdateModel();
		}

		// Change the skin and bodygroup to be correct for the mode and round
		m_pRoundSignModel->SetBodyGroup( "logos", nBodyGroup );
		m_pRoundSignModel->m_pModelInfo->m_nSkin = nSkin;
		// Make the model actually update with the new look
		m_pRoundSignModel->SetPanelDirty();
		m_pRoundSignModel->UpdateModel();
		// Play the sign drop anim
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudTournament_ShowRoundSign", false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 )
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
void CTFHudMatchStatus::InitPlayerList( SectionedListPanel *pPlayerList, int nTeam )
{
	pPlayerList->SetVerticalScrollbar( false );
	pPlayerList->RemoveAll();
	pPlayerList->RemoveAllSections();
	pPlayerList->AddSection( 0, "Players", TFPlayerSortFunc );
	pPlayerList->SetSectionAlwaysVisible( 0, true );
	pPlayerList->SetSectionDrawDividerBar( 0, false );
	pPlayerList->SetBorder( NULL );
	pPlayerList->SetMouseInputEnabled( false );
	pPlayerList->SetClickable( false );

	pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iAvatarWidth );
	pPlayerList->AddColumnToSection( 0, "spacer", "", 0, m_iSpacerWidth );

	// the player avatar is always a fixed size, so as we change resolutions we need to vary the size of the name column to adjust the total width of all the columns
	int nExtraSpace = pPlayerList->GetWide() - m_iAvatarWidth - m_iSpacerWidth - m_iNameWidth - ( 2 * SectionedListPanel::COLUMN_DATA_INDENT ); // the SectionedListPanel will indent the columns on either end by SectionedListPanel::COLUMN_DATA_INDENT 
	pPlayerList->AddColumnToSection( 0, "name", "", 0, m_iNameWidth + nExtraSpace );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::UpdatePlayerList()
{
	m_pPlayerListRed->RemoveAll();
	m_pPlayerListRed->ClearAllColorOverrideForCell();

	m_pPlayerListBlue->RemoveAll();
	m_pPlayerListBlue->ClearAllColorOverrideForCell();

	if ( !g_TF_PR )
		return;

	for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if ( g_PR->IsConnected( playerIndex ) )
		{
			SectionedListPanel *pPlayerList = NULL;
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

			pKeyValues->SetString( "name", g_TF_PR->GetPlayerName( playerIndex ) );

			UpdatePlayerAvatar( playerIndex, pKeyValues );

			int itemID = pPlayerList->AddItem( 0, pKeyValues );

			pPlayerList->SetItemFgColor( itemID, g_PR->GetTeamColor( nTeam ) );
			pPlayerList->SetItemBgColor( itemID, Color( 120, 120, 120, 80 ) );
			pPlayerList->SetItemBgHorizFillInset( itemID, m_iHorizFillInset );
			pPlayerList->SetItemFont( itemID, m_hPlayerListFont );

			pKeyValues->deleteThis();
		}
	}

	m_pPlayerListRed->SetSectionFgColor( 0, g_PR->GetTeamColor( TF_TEAM_RED ) );
	m_pPlayerListBlue->SetSectionFgColor( 0, g_PR->GetTeamColor( TF_TEAM_BLUE ) );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the player list
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	// Update their avatar
	if ( kv && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		player_info_t pi;
		if ( engine->GetPlayerInfo( playerIndex, &pi ) )
		{
			if ( pi.friendsID )
			{
				CSteamID steamIDForPlayer( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );

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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMatchStatus::UpdateTeamInfo()
{
	for ( int teamIndex = TF_TEAM_RED; teamIndex <= TF_TEAM_BLUE; teamIndex++ )
	{
		C_TFTeam *team = GetGlobalTFTeam( teamIndex );
		if ( team )
		{
			// choose dialog variables to set depending on team
			const char *pDialogVarTeamName = "";
			vgui::EditablePanel *pPanel = NULL;

			switch ( teamIndex )
			{
			case TF_TEAM_RED:
				pDialogVarTeamName = "redteamname";
				pPanel = m_pRedTeamPanel;
				break;
			case TF_TEAM_BLUE:
				pDialogVarTeamName = "blueteamname";
				pPanel = m_pBlueTeamPanel;
				break;
			default:
				Assert( false );
				break;
			}

			// set the team name
			if ( pPanel )
			{
				pPanel->SetDialogVariable( pDialogVarTeamName, team->Get_Localized_Name() );
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
