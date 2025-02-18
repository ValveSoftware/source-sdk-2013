//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "tf_pvp_rank_panel.h"
#include "tf_matchmaking_shared.h"
#include "basemodel_panel.h"
#include "vgui_controls/ProgressBar.h"
#include "tf_ladder_data.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include "tf_controls.h"
#include "vgui/ISurface.h"
#include "animation.h"
#include "clientmode_tf.h"
#include "vgui/IInput.h"
#include "vgui_controls/MenuItem.h"
#include "tf_gc_client.h"
#include "tf_xp_source.h"
#include "tf_item_inventory.h"
#include "tf_particlepanel.h"
#include "tf_rating_data.h"
#include "tf_gamerules.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


ConVar tf_xp_breakdown_interval( "tf_xp_breakdown_interval", "1.85", FCVAR_DEVELOPMENTONLY );
ConVar tf_xp_breakdown_lifetime( "tf_xp_breakdown_lifetime", "3.5", FCVAR_DEVELOPMENTONLY );

extern const char *s_pszMatchGroups[];

CPvPRankPanel::RatingState_t::RatingState_t( ETFMatchGroup eMatchGroup )
	: m_nStartRating( 0u )
	, m_nTargetRating( 0u )
	, m_nActualRating( 0u )
	, m_bCurrentDeltaViewed( true )
	, m_eMatchGroup( eMatchGroup )
	, m_bInitialized( false )
{
	Assert( m_eMatchGroup != k_eTFMatchGroup_Invalid );

	// Default to level 1's starting XP value
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
	if ( pMatchDesc && pMatchDesc->m_pProgressionDesc )
	{
		auto level = pMatchDesc->m_pProgressionDesc->GetLevelByNumber( 1 );
		m_nStartRating = level.m_nStartXP;
		m_nActualRating = m_nStartRating;
		m_nTargetRating = m_nStartRating;
	}

	ListenForGameEvent( "experience_changed" );
	ListenForGameEvent( "server_spawn" );
}

void CPvPRankPanel::RatingState_t::FireGameEvent( IGameEvent *pEvent )
{
	if ( FStrEq( pEvent->GetName(), "experience_changed" ) ) // For changing tf_progression_set_xp_to_level
	{
		UpdateRating( false );
	}
	else if ( FStrEq( pEvent->GetName(), "server_spawn" ) && GTFGCClientSystem()->BHaveLiveMatch()
		&& GTFGCClientSystem()->GetLiveMatchGroup() == m_eMatchGroup )
	{
		UpdateRating( false );
		// Acknowledge any outstanding rating sources when we start a match.  It looks really weird when
		// you see old match xp sources at the end of a match.
		GTFGCClientSystem()->AcknowledgePendingRatingAndSources( m_eMatchGroup );
	}
}

void CPvPRankPanel::RatingState_t::UpdateRating( bool bInitial )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
		return;

	// Update the starting rating if we've already lerped to show the progression
	if ( m_bCurrentDeltaViewed && GetCurrentRating() == m_nTargetRating )
		m_nStartRating = m_nTargetRating;

	uint32 nStartRating = 0u;
	uint32 nNewRating = 0u;

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
	// Should only be creating this object for matches that have progressions
	Assert( pMatchDesc && pMatchDesc->m_pProgressionDesc );
	if ( pMatchDesc && pMatchDesc->m_pProgressionDesc )
	{
		// Default to level 1's lowest XP value
		auto level = pMatchDesc->m_pProgressionDesc->GetLevelByNumber( 1 );
		nNewRating = level.m_nStartXP;
		nStartRating = level.m_nStartXP;

	
		// Starting rating
		CTFRatingData *pRating = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( steamapicontext->SteamUser()->GetSteamID(),
																						pMatchDesc->GetLastAckdDisplayRating() );
		nStartRating = pRating ? pRating->Obj().rating_primary() : 0u;

		// Actual, new rating
		pRating = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( steamapicontext->SteamUser()->GetSteamID(),
																						pMatchDesc->GetCurrentDisplayRating() );
		nNewRating = pRating ? pRating->Obj().rating_primary() : 0u; 
		
	}

	// If there's no change, don't do anything
	if ( nNewRating != m_nActualRating || bInitial )
	{
		m_nActualRating = nNewRating;
		if ( bInitial )
		{
			m_nStartRating = nStartRating;
		}
		else
		{
			m_nStartRating = Min( m_nStartRating, nStartRating );
		}
		m_bCurrentDeltaViewed = false;
		m_progressTimer.Invalidate();
	}

	if ( bInitial )
	{
		m_progressTimer.Start( 1.f );
		m_bCurrentDeltaViewed = true;
		m_nTargetRating = m_nStartRating;
		m_bInitialized = true;
	}
}

uint32 CPvPRankPanel::RatingState_t::GetCurrentRating() const
{
	float flTimeProgress = 0.f;
	if ( m_progressTimer.HasStarted() )
	{
		flTimeProgress = Clamp( m_progressTimer.GetElapsedTime(), 0.f, m_progressTimer.GetCountdownDuration() );
		flTimeProgress = Gain( flTimeProgress / m_progressTimer.GetCountdownDuration(), 0.9f );
	}

	return RemapValClamped( flTimeProgress, 0.f, 1.f, m_nStartRating, m_nTargetRating );
}

bool CPvPRankPanel::RatingState_t::BeginRatingDeltaLerp()
{
	if ( !m_bCurrentDeltaViewed )
	{
		m_bCurrentDeltaViewed = true;
		// Change our target
		m_nTargetRating = m_nActualRating;
		m_progressTimer.Start( 5.f );

		return true;
	}

	return false;
}

void CPvPRankPanel::RatingState_t::SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) 
{
	if ( pObject->GetTypeID() == CSOTFLadderData::k_nTypeID )
	{
		CSOTFLadderData* pLadderObject = (CSOTFLadderData*)pObject;
		if( (ETFMatchGroup)pLadderObject->Obj().match_group() != m_eMatchGroup )
			return;

		// We'll get a eSOCacheEvent_Incremental when we're actually creating the 
		// first CSOTFLadderData object.  We dont want that to come through as
		// "initializing" because it'll skip the leveling effects
		UpdateRating( eEvent == eSOCacheEvent_ListenerAdded );
		m_bInitialized = true;
	}

	if ( pObject->GetTypeID() == CXPSource::k_nTypeID )
	{
		CXPSource *pXPSource = (CXPSource*)pObject;
		if ( pXPSource->Obj().match_group() == m_eMatchGroup )
		{
			UpdateRating( false );
			m_bInitialized = true;
		}
	}
}

void CPvPRankPanel::RatingState_t::SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CSOTFLadderData::k_nTypeID )
	{
		CSOTFLadderData* pLadderObject = (CSOTFLadderData*)pObject;
		if( (ETFMatchGroup)pLadderObject->Obj().match_group() != m_eMatchGroup )
			return;

		// If the GC comes on after we've already subscribed to the cache,
		// eSOCacheEvent_Subscribed when the object is created.  This one
		// we want to skip the effects.
		UpdateRating( eEvent == eSOCacheEvent_Subscribed );
	}

	if ( pObject->GetTypeID() == CTFRatingData::k_nTypeID )
	{
		auto pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
		if ( !pMatchDesc )
			return;

		CTFRatingData* pRatingObject = (CTFRatingData*)pObject;
		if ( pRatingObject->Obj().rating_type() == pMatchDesc->GetLastAckdDisplayRating() ||
			 pRatingObject->Obj().rating_type() == pMatchDesc->GetCurrentDisplayRating() )
			UpdateRating( eEvent == eSOCacheEvent_Subscribed );
	}

	if ( pObject->GetTypeID() == CXPSource::k_nTypeID )
	{
		CXPSource *pXPSource = (CXPSource*)pObject;
		if ( pXPSource->Obj().match_group() == m_eMatchGroup )
		{
			UpdateRating( false );
		}
	}
}

class CMiniPvPRankPanel : public CPvPRankPanel
{
public:
	CMiniPvPRankPanel( Panel* pParent, const char* pszPanelName ) : CPvPRankPanel( pParent, pszPanelName ) 
	{} 

private:
	virtual KeyValues* GetConditions() const OVERRIDE
	{
		KeyValues* pConditions = new KeyValues( "conditions" );
		pConditions->AddSubKey( new KeyValues( "if_mini" ) );

		return pConditions;
	}
};


DECLARE_BUILD_FACTORY( CMiniPvPRankPanel );
DECLARE_BUILD_FACTORY( CPvPRankPanel );

struct RankRevealState_t
{
	bool m_bNeedsRankReveal = false;
	bool m_bLastSeenInPlacement = true;
	bool m_bInitialized = false;
};

static RankRevealState_t s_rankReavealState[ ETFMatchGroup_ARRAYSIZE ];

CPvPRankPanel::CPvPRankPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
	, m_eMatchGroup( k_eTFMatchGroup_Invalid )
	, m_pProgressionDesc( NULL )
	, m_pContinuousProgressBar( NULL )
	, m_pModelPanel( NULL )
	, m_pXPBar( NULL )
	, m_pBGPanel( NULL )
	, m_nLastLerpRating( 0 )
	, m_nLastSeenLevel( 0 )
	, m_bInitializedBaseState( false )
{
	m_pModelContainer = new EditablePanel( this, "ModelContainer" );
	m_pModelPanel = new CBaseModelPanel( m_pModelContainer, "RankModel" );
	m_pModelPanel->AddActionSignalTarget( this );

	ListenForGameEvent( "begin_xp_lerp" );
	ListenForGameEvent( "gc_new_session" );
	ListenForGameEvent( "mainmenu_stabilized" );
}

void CPvPRankPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues* pConditions = GetConditions();

	LoadControlSettings( GetResFile(), NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
		pConditions = NULL;
	}

	m_pBGPanel = FindControl< EditablePanel >( "BGPanel", true );
	m_pContinuousProgressBar = FindControl< ContinuousProgressBar >( "ContinuousProgressBar", true );
	m_pXPBar = FindControl< EditablePanel >( "XPBar", true );


	m_pModelButton = FindControl< Button >( "MedalButton", true );
	if ( m_pModelButton )
	{
		m_pModelButton->AddActionSignalTarget( this );
		m_pModelButton->SetButtonActivationType( Button::ACTIVATE_ONPRESSED );
	}

}

void CPvPRankPanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	SetMatchGroup( (ETFMatchGroup)StringFieldToInt( inResourceData->GetString( "matchgroup" ), s_pszMatchGroups, (int)ETFMatchGroup_ARRAYSIZE, false ) );
}

void CPvPRankPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_bRevealingRank = s_rankReavealState[ m_eMatchGroup ].m_bNeedsRankReveal;

	SetControlVisible( "BGPanel", m_bShowProgress, true );
	SetControlVisible( "ModelContainer", m_bShowModel, true );
	m_pBGPanel->SetControlVisible( "NameLabel", m_bShowName, true );
	SetControlVisible( "StatsContainer", !BIsInPlacement() && !m_bRevealingRank, true );

	if ( !m_pProgressionDesc || !m_pMatchDesc )
		return;

	EditablePanel* pStatsContainer = FindControl< EditablePanel >( "Stats", true );
	if ( !pStatsContainer )
		return;

	if ( !m_pModelPanel )
		return;

	if ( !m_pXPBar )
		return;

	ProgressBar* pProgressBar = FindControl< ProgressBar > ( "ProgressBar", true );
	if ( !pProgressBar )
		return;

	if ( !m_pContinuousProgressBar )
		return;	

	// Chop up the progress bar into 10th's and use it as a mask overtop
	pProgressBar->SetSegmentInfo( ( pProgressBar->GetWide() / 10 ) - ( ( 9 * 2 ) / 10 ) , 2 );
	
	const LevelInfo_t& levelCur = GetLevel( m_bInstantlyUpdate );
	m_pMatchDesc->SetupBadgePanel( m_pModelPanel, levelCur, ClientSteamContext().GetLocalPlayerSteamID(), BIsInPlacement() || m_bRevealingRank );

	// Update labels
	if ( m_bShowRating )
	{
		const RatingState_t& ratingState = GetRatingState();
		uint32 nCurrentRating = ratingState.GetCurrentRating();
		UpdateRatingControls( ratingState.GetStartRating(), nCurrentRating, levelCur );
	}
	UpdateRankControls( levelCur );

	SetMatchStats();
}

const LevelInfo_t& CPvPRankPanel::GetLevel( bool bCurrent ) const
{
	// Sticky ranks just use the ranks in our rating data
	if ( m_pMatchDesc->BUsesStickyRanks() )
	{
		EMMRating eRating = bCurrent ? m_pMatchDesc->GetCurrentDisplayRank() 
									 : m_pMatchDesc->GetLastAckdDisplayRank();
		CTFRatingData* pRatingData = SteamUser() ? CTFRatingData::YieldingGetPlayerRatingDataBySteamID( SteamUser()->GetSteamID(), eRating )
												 : NULL;
		uint32 nCurrentRank = pRatingData ? pRatingData->GetRatingData().unRatingPrimary : 0;
		return m_pProgressionDesc->GetLevelByNumber( nCurrentRank );
	}
	else
	{
		// Non-sticky ranks we want to show the level based on what visually lines up with our displayed rating
		uint32 nCurrentRating = bCurrent ? GetRatingState().GetTargetRating() : GetRatingState().GetCurrentRating();
		return m_pProgressionDesc->GetLevelForRating( nCurrentRating );
	}
}

void CPvPRankPanel::OnThink()
{
	BaseClass::OnThink();

	if ( !m_pMatchDesc )
		return;

	if ( TFGameRules() && TFGameRules()->GetCurrentMatchGroup() != m_pMatchDesc->m_eMatchGroup )
		return;

	if ( !m_bShowRating )
		return;

	if ( !m_pContinuousProgressBar || !m_pXPBar || !m_pModelPanel || !m_pProgressionDesc || !m_pBGPanel )
		return;

	const RatingState_t& ratingState = GetRatingState();
	if ( !ratingState.IsInitialized() )
		return;	

	if ( !m_bInitializedBaseState )
	{
		UpdateBaseState();
		m_bInitializedBaseState = true;
	}

	uint32 nCurrentRating = ratingState.GetCurrentRating();

	// Check if the last XP we lerp'd to isn't the current XP.  If it's not, then we want to animate over to it.
	if ( ratingState.GetTargetRating() != m_nLastLerpRating )
	{
		uint32 nPrevXP = ratingState.GetStartRating();
		const LevelInfo_t& levelCur = m_pMatchDesc->BUsesStickyRanks() ? GetLevel( true ) 
																	   : m_pProgressionDesc->GetLevelForRating( nCurrentRating );

		UpdateRatingControls( nPrevXP, nCurrentRating, levelCur );
		UpdateRankControls( levelCur );

		// We only want to do level up effects if we are thinking (visible) when the current XP passes the level boundary
		if ( m_nLastLerpRating != ratingState.GetStartRating() && !m_pMatchDesc->BLocalPlayerIsInPlacement() )
		{
			const LevelInfo_t& levelPrevThink = m_pMatchDesc->BUsesStickyRanks() ? GetLevel( false )
																				 : m_pProgressionDesc->GetLevelForRating( m_nLastLerpRating );
			if ( levelCur.m_nLevelNum > levelPrevThink.m_nLevelNum ) // Level up :)
			{
				PlayLevelUpEffects( levelCur );
			}
			else if ( levelCur.m_nLevelNum < levelPrevThink.m_nLevelNum ) // Level down :(
			{
				PlayLevelDownEffects( levelCur );
			}
		}

		m_nLastLerpRating = nCurrentRating;
	}
	else if ( !m_pMatchDesc->BUsesStickyRanks() )
	{
		// Our last lerp XP is caught up with current XP, but our last seen level is not up to date with what
		// our current level actually is.  This can happen if we haven't been thinking, but we did level up somewhere
		// else.  In this case, we need to update our badge.
		const LevelInfo_t& levelCur = GetLevel( true );
		if ( m_nLastSeenLevel != levelCur.m_nLevelNum )
		{
			m_nLastSeenLevel = levelCur.m_nLevelNum;
			m_pMatchDesc->SetupBadgePanel( m_pModelPanel, levelCur, ClientSteamContext().GetLocalPlayerSteamID(), BIsInPlacement()  );
		}
	}
}

void CPvPRankPanel::UpdateRatingControls( uint32 nPreviousRating, uint32 nCurrentRating, const LevelInfo_t& levelCurrent )
{
	if ( m_bShowRating && m_pContinuousProgressBar )
	{
		// Calculate progress bar percentages.  PrevBarProgress is the bar that shows where you started.
		// CurrentBarProgress is the bar that lerps from the start to your actual, current XP
		float flPrevBarProgress		= RemapValClamped( (float)nPreviousRating,	  (float)levelCurrent.m_nStartXP, (float)levelCurrent.m_nEndXP, 0.f, 1.f );
		float flCurrentBarProgress	= RemapValClamped( (float)nCurrentRating, (float)levelCurrent.m_nStartXP, (float)levelCurrent.m_nEndXP, 0.f, 1.f );

		m_pContinuousProgressBar->SetPrevProgress( flPrevBarProgress );
		m_pContinuousProgressBar->SetProgress( flCurrentBarProgress );
	}

	if ( !m_pXPBar )
		return;

	// We only set these up if we're actually showing rating
	if ( m_bShowRating )
	{
		wchar_t wszRating[ 128 ];
		auto lambdaRatingString = [ this, &wszRating ]( const char* pszToken, int nRating )
		{
			V_swprintf_safe( wszRating, L"%d %ls", nRating, g_pVGuiLocalize->Find( m_pProgressionDesc->GetRankUnitsLocToken() ) );
			return wszRating;
		};

		m_pXPBar->SetDialogVariable( "current_xp", lambdaRatingString( "TF_Competitive_Rating_Current", nCurrentRating ) );

		if ( levelCurrent.m_nLevelNum < m_pProgressionDesc->GetNumLevels() )
		{
			m_pXPBar->SetDialogVariable( "next_level_xp", lambdaRatingString( "TF_Competitive_Rating", levelCurrent.m_nEndXP ) );
		}
		else // Hide the next level XP value at max level
		{
			m_pXPBar->SetDialogVariable( "next_level_xp", "" );
		}
	}
}

void CPvPRankPanel::UpdateRankControls( const LevelInfo_t& levelCurrent )
{
	if ( !m_pXPBar )
		return;


	
	if ( m_pBGPanel )
	{
		const char* pszLevelVar = m_bShowType ? "desc2" : "desc1";
		const char* pszTypeVar = m_bShowType ? "desc1" : NULL;

		m_pBGPanel->SetDialogVariable( pszTypeVar, g_pVGuiLocalize->Find( m_pMatchDesc->GetNameLocToken() ) );

		if ( BIsInPlacement() )
		{
			CSteamID steamID = steamapicontext->SteamUser() ? steamapicontext->SteamUser()->GetSteamID()
															: CSteamID();
			// If in placement, then cook up our "Win X more matches to earn a rank!" string
			int nPlacementsToGo = m_pMatchDesc->GetNumPlacementMatchesToGo( steamID );

			static wchar_t wszOutString[ 256 ];
			const wchar_t *wpszFormat = g_pVGuiLocalize->Find( nPlacementsToGo == 1 ? "#TF_Competitive_Placements_Singular" 
																				: "#TF_Competitive_Placements_Multiple" );
			g_pVGuiLocalize->ConstructString_safe( wszOutString,
													wpszFormat,
													1,
													CStrAutoEncode( CFmtStr( "%d", nPlacementsToGo ) ).ToWString());

			m_pBGPanel->SetDialogVariable( pszLevelVar, wszOutString );
		}
		else
		{
			// Get their rank title's string
			wchar_t wszLevelString[ 128 ];
			m_pProgressionDesc->GetLocalizedLevelTitle( levelCurrent, wszLevelString, 128 );
			m_pBGPanel->SetDialogVariable( pszLevelVar, wszLevelString );
		}

		m_pBGPanel->SetControlVisible( "DescLine1", true ); // Gets used by level or type
		m_pBGPanel->SetControlVisible( "DescLine2", m_bShowType ); // Possibly used by level if type is enabled
	}

	if ( m_bRevealingRank )
	{
		if ( m_pBGPanel )
		{
			m_pBGPanel->SetAlpha( 0 );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pBGPanel, "alpha", 255, 2.f, 1.f, AnimationController::INTERPOLATOR_LINEAR, 0.f, true, false ); 
		}
	}
}

void CPvPRankPanel::OnCommand( const char *command )
{
	if ( FStrEq( "medal_clicked", command ) )
	{
		int nMouseX, nMouseY;
		g_pVGuiInput->GetCursorPosition( nMouseX, nMouseY );

		// Detect if they actually clicked in the circle of the medal
		if ( m_pModelButton )
		{
			int nX,nY,nWide,nTall;
			m_pModelButton->GetBounds( nX, nY, nWide, nTall ); 
			m_pModelButton->GetParent()->LocalToScreen( nX, nY );

			Vector2D vCenter( nX + ( nWide * 0.5f ), nY + ( nTall * 0.5f ) );
			Vector2D vMouse( nMouseX, nMouseY );
			float flLengthSq = ( vMouse - vCenter ).LengthSqr();
			float flRadiusSq = ( nWide * 0.5f ) * ( nWide * 0.5f );
			if ( flLengthSq > ( flRadiusSq ) )
				return;
		}

		// Default effects
		bool bCrit = RandomInt( 0, 9 ) == 0;
		const char *pszSeqName = bCrit ? "click_B" : "click_A";
		const char *pszSoundName = NULL;
		
		// Placement gets its own sounds
		if ( BIsInPlacement() )
		{
			pszSoundName = bCrit ? "MatchMaking.MedalClickRankUnknownRare" : "MatchMaking.MedalClickRankUnknown";
		}
		else
		{
			pszSoundName = "ui/mm_medal_click.wav";

			if ( bCrit )
			{
				int nLogoValue = 0;
				// This could be better...
// 				CTFPlayerInventory *pInv = TFInventoryManager()->GetLocalTFInventory();
// 				static CSchemaItemDefHandle pItemDef_ActivatedCampaign3Pass( "Activated Campaign 3 Pass" );
// 				if ( pInv && pInv->GetFirstItemOfItemDef( pItemDef_ActivatedCampaign3Pass->GetDefinitionIndex() ) != NULL  )
// 				{
// 					nLogoValue = 1;
// 				}

				// CRIT!
				pszSoundName = nLogoValue == 1 ? "MatchMaking.MedalClickRareYeti" : "MatchMaking.MedalClickRare";
			}
		}
	
		m_pModelPanel->PlaySequence( pszSeqName );
		PlaySoundEntry( pszSoundName );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pModelContainer, "PvPRankModelClicked", false);
		
		return;
	}
	else if ( FStrEq( "begin_xp_lerp", command ) )
	{
		BeginRatingLerp();
		return;
	}
	else if ( FStrEq( "update_base_state", command ) )
	{
		UpdateBaseState();
		return;
	}
	BaseClass::OnCommand( command );
}

void CPvPRankPanel::FireGameEvent( IGameEvent *pEvent )
{
	// This is really only for tf_test_pvp_rank_xp_change
	if ( FStrEq( pEvent->GetName(), "begin_xp_lerp" ) || 
		 FStrEq( pEvent->GetName(), "gc_new_session" ) ||
		 FStrEq( pEvent->GetName(), "mainmenu_stabilized" ) )
	{
		if ( !m_bInitializedBaseState )
		{
			UpdateBaseState();
		}
		else
		{
			BeginRatingLerp();
		}
	}
}

void CPvPRankPanel::SetVisible( bool bVisible )
{
	 if ( IsVisible() != bVisible )
	 {
		 UpdateBaseState();
	 }

	 BaseClass::SetVisible( bVisible );
}

void UpdateRevealState( ETFMatchGroup eMatchGroup, bool bInitial = false )
{
	auto& revealState = s_rankReavealState[ eMatchGroup ];

	// Don't do anything until we get our initialize
	if ( !revealState.m_bInitialized && !bInitial )
		return;

	bool bCurrentlyInPlacement = false;

	auto pMatchGroup = GetMatchGroupDescription( eMatchGroup );
	if ( pMatchGroup )
	{
		bCurrentlyInPlacement = pMatchGroup->BLocalPlayerIsInPlacement();
	}

	// Initialize if we haven't yet
	if ( !revealState.m_bInitialized )
	{
		revealState.m_bInitialized = true;
		revealState.m_bNeedsRankReveal = false;
		revealState.m_bLastSeenInPlacement = bCurrentlyInPlacement;
	}

	// If we were in placement, but aren't now then we need to reveal our rank
	if ( !bCurrentlyInPlacement && revealState.m_bLastSeenInPlacement )
	{
		revealState.m_bNeedsRankReveal = true;
	}

	revealState.m_bLastSeenInPlacement = bCurrentlyInPlacement;
}


void CPvPRankPanel::BeginRatingLerp()
{
	if ( !m_pMatchDesc )
		return;

	if ( !IsVisible() && !m_bShowSourcesWhenHidden )
	{
		LevelInfo_t levelCur = GetLevel( true );

		m_pMatchDesc->SetupBadgePanel( m_pModelPanel, levelCur, ClientSteamContext().GetLocalPlayerSteamID(), BIsInPlacement() || m_bRevealingRank );

		// Update progress bars and labels
		if ( m_bShowRating )
		{
			m_nLastLerpRating = GetRatingState().GetCurrentRating();
			UpdateRatingControls( GetRatingState().GetStartRating(), m_nLastLerpRating, levelCur );
		}
		UpdateRankControls( levelCur );

		return;
	}

	if ( m_bShowRating )
	{
		RatingState_t& xpstate = GetRatingState();
		if ( xpstate.BeginRatingDeltaLerp() && !BIsInPlacement() )
		{
			// Play sounds if this is a change
			if ( xpstate.GetTargetRating() > xpstate.GetStartRating() )
			{
				PlaySoundEntry( "MatchMaking.RankProgressTickUp" );
			}
			else if ( xpstate.GetTargetRating() < xpstate.GetStartRating() )
			{
				PlaySoundEntry( "MatchMaking.RankProgressTickDown" );
			}
		}

		if ( !steamapicontext || !steamapicontext->SteamUser() )
			return;
	
		GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamapicontext->SteamUser()->GetSteamID() );
		if ( !pSOCache )
			return;

		GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CXPSource::k_nTypeID );
		if ( pTypeCache && !BIsInPlacement() )
		{
			CUtlVector< CXPSource* > vecSources;

			// Grab all the XP sources we want to show
			for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
			{
				CXPSource *pXPSource = (CXPSource*)pTypeCache->GetObject( i );

				if ( pXPSource->Obj().match_group() == m_eMatchGroup )
				{
					vecSources.AddToTail( pXPSource );
				}
			}
					
			// Sort them so users get a consistent experience
			vecSources.SortPredicate( []( const CXPSource* pLeft, const CXPSource* pRight )
			{
				return pLeft->Obj().type() < pRight->Obj().type();
			} );
					
			// Show the sources
			FOR_EACH_VEC( vecSources, i )
			{
				CXPSource* pSource = vecSources[ i ];
				auto& source = pSource->Obj();

				static wchar_t wszOutString[ 128 ];
				wchar_t wszCount[ 16 ];
				_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", source.amount() );
				const wchar_t *wpszFormat = g_pVGuiLocalize->Find( g_XPSourceDefs[ source.type() ].m_pszFormattingLocToken );
				g_pVGuiLocalize->ConstructString_safe( wszOutString,
													   wpszFormat,
													   3,
													   g_pVGuiLocalize->Find( g_XPSourceDefs[ source.type() ].m_pszTypeLocToken ),
													   wszCount,
													   g_pVGuiLocalize->Find( m_pProgressionDesc->GetRankUnitsLocToken() ) );

				int nX, nY;
				m_pXPBar->GetPos( nX, nY );
				nX += m_pXPBar->GetWide() / 2; // Center it
				nY -= YRES( 10 ); // Up a bit so it's not ON the bar
				m_pXPBar->ParentLocalToScreen( nX, nY );

				CreateScrollingIndicator( nX,
										  nY,
										  wszOutString,
										  g_XPSourceDefs[ source.type() ].m_pszSoundName,
										  i * tf_xp_breakdown_interval.GetFloat(),
										  0,
										  m_pMatchDesc->BUsesStickyRanks() ? 0 : -25,
										  source.amount() >= 0 );

			}
		}
	}

	// Sticky ranks just do their effects right away since they dont lerp anything meaningful for ranks
	if ( m_pMatchDesc->BUsesStickyRanks() )
	{
		const LevelInfo_t& oldLevel = GetLevel( false );
		const LevelInfo_t& newLevel = GetLevel( true );

		if ( m_bShowModel )
		{
			if ( ( !BIsInPlacement() && newLevel.m_nLevelNum > oldLevel.m_nLevelNum ) || m_bRevealingRank ) // Level up :)
			{
				PlayLevelUpEffects( newLevel );
				s_rankReavealState[ m_eMatchGroup ].m_bNeedsRankReveal = false;
			}
			else if ( !BIsInPlacement() && newLevel.m_nLevelNum < oldLevel.m_nLevelNum ) // Level down :(
			{
				PlayLevelDownEffects( newLevel );
			}
		}

		UpdateRankControls( newLevel );
	}

	
	UpdateRevealState( m_eMatchGroup );
	m_bRevealingRank = false;
	GTFGCClientSystem()->AcknowledgePendingRatingAndSources( m_eMatchGroup );
}

void CPvPRankPanel::UpdateBaseState()
{
	if ( !m_pProgressionDesc || m_pModelPanel == NULL )
		return;

	// Make sure our reveal state is initialized
	if ( !s_rankReavealState[ m_eMatchGroup ].m_bInitialized )
	{
		UpdateRevealState( m_eMatchGroup );
	}

	// Set our "last seen" variables so things dont try to interpolate
	LevelInfo_t levelCur = GetLevel( false );
	m_nLastSeenLevel = levelCur.m_nLevelNum;

	m_pMatchDesc->SetupBadgePanel( m_pModelPanel, levelCur, ClientSteamContext().GetLocalPlayerSteamID(), BIsInPlacement() || m_bRevealingRank );

	// Update progress bars and labels
	if ( m_bShowRating )
	{
		m_nLastLerpRating = GetRatingState().GetCurrentRating();
		UpdateRatingControls( GetRatingState().GetStartRating(), m_nLastLerpRating, levelCur );
	}
	UpdateRankControls( levelCur );
}

void CPvPRankPanel::OnAnimEvent( KeyValues *pParams )
{
	// This gets fired by the model panel that has the badge model in it when we want
	// to do our bodygroup changes.  This is so we can time it to when the model is doing
	// a flashy maneuver to mask the bodygroup change pop
	if ( FStrEq( pParams->GetString( "name" ), "AE_CL_BODYGROUP_SET_VALUE" ) && m_pProgressionDesc )
	{
		const RatingState_t& ratingState = GetRatingState();
		const LevelInfo_t& levelCur = m_pMatchDesc->BUsesStickyRanks() ? GetLevel( true ) 
									: m_pProgressionDesc->GetLevelForRating( ratingState.GetCurrentRating() );
		m_pMatchDesc->SetupBadgePanel( m_pModelPanel, levelCur, ClientSteamContext().GetLocalPlayerSteamID(), BIsInPlacement() );
	}
}

void CPvPRankPanel::PlayLevelUpEffects( const LevelInfo_t& level ) const
{
	if ( m_bShowModel )
	{
		m_pModelPanel->PlaySequence( "level_up" );
		PlaySoundEntry( level.m_pszLevelUpSound );
		EditablePanel* pModelContainer = const_cast< CPvPRankPanel* >( this )->FindControl< EditablePanel >( "ModelContainer" );
		if ( pModelContainer )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pModelContainer, "PvPRankLevelUpModel", false );
		}
	}

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pXPBar, "PvPRankLevelUpXPBar", false);

	wchar_t wszOutString[ 128 ];
	m_pProgressionDesc->GetLocalizedLevelTitle( level, wszOutString, 128 );
	if ( m_pBGPanel )
	{
		m_pBGPanel->SetDialogVariable( "level", wszOutString );
	}
}

void CPvPRankPanel::PlayLevelDownEffects( const LevelInfo_t& level ) const
{
	if ( m_bShowModel )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pXPBar, "PvPRankLevelDownXPBar", false);
		m_pModelPanel->PlaySequence( "level_down" );
		EditablePanel* pModelContainer = const_cast< CPvPRankPanel* >( this )->FindControl< EditablePanel >( "ModelContainer" );
		if ( pModelContainer )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pModelContainer, "PvPRankLevelDownModel", false );
		}
	}

	wchar_t wszOutString[ 128 ];
	m_pProgressionDesc->GetLocalizedLevelTitle( level, wszOutString, 128 );
	if ( m_pBGPanel )
	{
		m_pBGPanel->SetDialogVariable( "level", wszOutString );
	}
}

void CPvPRankPanel::SetMatchGroup( ETFMatchGroup eMatchGroup )
{
	if ( m_eMatchGroup != eMatchGroup )
	{
		m_eMatchGroup = eMatchGroup;
		m_pProgressionDesc = NULL;

		m_pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
		if ( m_pMatchDesc )
		{
			// Snag the progression desc.  We use it often
			m_pProgressionDesc = m_pMatchDesc->m_pProgressionDesc;

			// Only show rating if displaying rating is not invalid
			m_bShowRating = m_pMatchDesc->GetCurrentDisplayRating() != k_nMMRating_Invalid;
		}

		Assert( m_pProgressionDesc );

		// Many things needs to change
		InvalidateLayout( true, true );
		UpdateBaseState();
	}
}

void CPvPRankPanel::SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFRatingData::k_nTypeID )
	{
		UpdateBaseState();
		SetMatchStats();

		for( int eMatchGroup = k_eTFMatchGroup_First; eMatchGroup < ETFMatchGroup_ARRAYSIZE; ++eMatchGroup )
		{
			UpdateRevealState( (ETFMatchGroup)eMatchGroup, eEvent == eSOCacheEvent_Subscribed 
											|| eEvent == eSOCacheEvent_ListenerAdded );
		}
		m_bRevealingRank = s_rankReavealState[ m_eMatchGroup ].m_bNeedsRankReveal;
	}
}

void CPvPRankPanel::SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFRatingData::k_nTypeID )
	{
		UpdateBaseState();
		SetMatchStats();

		for( int eMatchGroup = k_eTFMatchGroup_First; eMatchGroup < ETFMatchGroup_ARRAYSIZE; ++eMatchGroup )
		{
			UpdateRevealState( (ETFMatchGroup)eMatchGroup, eEvent == eSOCacheEvent_Subscribed 
							   || eEvent == eSOCacheEvent_ListenerAdded );
		}
		m_bRevealingRank = s_rankReavealState[ m_eMatchGroup ].m_bNeedsRankReveal;

		if ( m_bInstantlyUpdate )
			InvalidateLayout();
	}
}

void CPvPRankPanel::SetMatchStats( void )
{
	EditablePanel* pStatsContainer = FindControl< EditablePanel >( "Stats", true );
	if ( !pStatsContainer )
		return;

	const CSOTFLadderData *pData = GetLocalPlayerLadderData( m_eMatchGroup );

	// Update all stats.  Default to 0 in case we don't have ladder data
	int nGames = 0, nKills = 0, nDeaths = 0, nDamage = 0, nHealing = 0, nSupport = 0, nScore = 0;

	if ( pData )
	{
		nGames = pData->Obj().games();
		nKills = pData->Obj().kills();
		nDeaths = pData->Obj().deaths();
		nDamage = pData->Obj().damage();
		nHealing = pData->Obj().healing();
		nSupport = pData->Obj().support();
		nScore = pData->Obj().score();
	}

	if ( steamapicontext && steamapicontext->SteamFriends() )
	{
		m_pBGPanel->SetDialogVariable( "name", steamapicontext->SteamFriends()->GetPersonaName() );
	}
	else
	{
		m_pBGPanel->SetDialogVariable( "name", "" );
	}

	pStatsContainer->SetDialogVariable( "stat_games", LocalizeNumberWithToken( "TF_Competitive_Games", nGames ) );
	pStatsContainer->SetDialogVariable( "stat_kills", LocalizeNumberWithToken( "TF_Competitive_Kills", nKills ) );
	pStatsContainer->SetDialogVariable( "stat_deaths", LocalizeNumberWithToken( "TF_Competitive_Deaths", nDeaths ) );
	pStatsContainer->SetDialogVariable( "stat_damage", LocalizeNumberWithToken( "TF_Competitive_Damage", nDamage ) );
	pStatsContainer->SetDialogVariable( "stat_healing", LocalizeNumberWithToken( "TF_Competitive_Healing", nHealing ) );
	pStatsContainer->SetDialogVariable( "stat_support", LocalizeNumberWithToken( "TF_Competitive_Support", nSupport ) );
	pStatsContainer->SetDialogVariable( "stat_score", LocalizeNumberWithToken( "TF_Competitive_Score", nScore ) );
}

CPvPRankPanel::RatingState_t& CPvPRankPanel::GetRatingState() const
{
	Assert( m_bShowRating );

	// Singletons for each RatingState_t
	static CUtlMap< ETFMatchGroup, CPvPRankPanel::RatingState_t* > s_mapRatingStates( DefLessFunc( ETFMatchGroup ) );

	auto idx = s_mapRatingStates.Find( m_eMatchGroup );
	if ( idx == s_mapRatingStates.InvalidIndex() )
	{
		idx = s_mapRatingStates.Insert( m_eMatchGroup, new CPvPRankPanel::RatingState_t( m_eMatchGroup ) );
	}

	return *s_mapRatingStates[ idx ];
}

const char* CPvPRankPanel::GetResFile() const
{
	return m_pProgressionDesc ? m_pProgressionDesc->m_pszProgressionResFile : "resource/ui/PvPCompRankPanel.res";
}

KeyValues* CPvPRankPanel::GetConditions() const
{
	return NULL;
}

bool CPvPRankPanel::BIsInPlacement() const
{
	return m_pMatchDesc &&
		   m_pMatchDesc->BUsesPlacementMatches() &&
		   s_rankReavealState[ m_eMatchGroup ].m_bLastSeenInPlacement;
}