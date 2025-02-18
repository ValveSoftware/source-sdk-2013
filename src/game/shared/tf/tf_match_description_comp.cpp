//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_match_description_shared.h"
#include "tf_progression_description.h"
#include "tf_matchmaking_shared.h"
#include "tf_progression.h"

#if   defined GAME_DLL
	#include "tf_rating_data.h"
	#include "tf_gamerules.h"
#elif defined CLIENT_DLL
	#include "tf_rating_data.h"
	#include "tf_gamerules.h"
#endif


class CLadderMatchGroupDescription : public IMatchGroupDescription
{
public:
	CLadderMatchGroupDescription( ETFMatchGroup eMatchGroup, ConVar* pmm_match_group_size )
		: IMatchGroupDescription( eMatchGroup )
	{
		m_pProgressionDesc = GetProgressionDesc( k_eProgression_Glicko );

		// Comp settings
		m_bActive						= true;
		m_eLateJoinMode					= eMatchMode_MatchMaker_LateJoinMatchBased;
		m_ePenaltyPool					= eMMPenaltyPool_Ranked;
		m_bUsesSkillRatings				= true;
		m_bSupportsLowPriorityQueue		= true;
		m_bUseMatchHud					= true;
		m_pszExecFileName				= "server_competitive.cfg";
		m_pmm_match_group_size			= pmm_match_group_size;
		m_pszMatchEndKickWarning		= "#TF_Competitive_GameOver";
		m_pszMatchStartSound			= "MatchMaking.RoundStart";
		m_bUseMatchSummaryStage			= true;
		m_bDistributePerformanceMedals	= true;
		m_bUseFirstBlood				= true;
		m_bUseReducedBonusTime			= true;
		m_bRandomWeaponCrits			= false;
		m_bFixedWeaponSpread			= true;
		m_bForceClientSettings			= true;
		m_bAllowDrawingAtMatchSummary	= true;
		m_bUsesSurveys					= true;
		m_bStrictMatchmakerScoring		= true;
		m_pszModeNameLocToken			= "#TF_Matchmaking_HeaderCompetitive";
		m_pszRichPresenceLocToken		= "Competitive6v6";
		m_eMatchType					= MATCH_TYPE_COMPETITIVE;
		m_nNumWinsToExitPlacement		= 10;
		m_eCurrentDisplayRating			= k_nMMRating_6v6_GLICKO;
		m_eLastAckdDisplayRating		= k_nMMRating_6v6_GLICKO_PlayerAcknowledged;
		m_eCurrentDisplayRank			= k_nMMRating_6v6_Rank;
		m_eLastAckdDisplayRank			= k_nMMRating_6v6_Rank_PlayerAcknowledged;
		m_bUsesStickyRanks				= true;
		m_bUsesStrictAbandons			= true;
		m_bRequiresCompetitiveAccess	= true;
		m_eLeaderboard					= k_eMatchGroupLeaderboard_Ladder6v6;
		m_bUsesStrictSpectatorRules		= true;

		// Steam stats for this group. Tracking these for Steam Summer Sale 2019, may be unused after that?
	}



#if defined( GAME_DLL ) || defined( CLIENT_DLL )
	bool BMatchIsHighSkill() const
	{
		// Get the average rating from the match
#ifdef GAME_DLL
		const float flNormalizedRating = GTFGCClientSystem()->GetMatch() ? GTFGCClientSystem()->GetMatch()->m_uInitialAverageMMRating : 0.f;
#else
		// TODO: the GC Client should be wrapping this field, similar to match ID and other lobby fields -- the lobby
		//       can go away without the match going away (or the server should be communicating this value)
		const float flNormalizedRating = GTFGCClientSystem()->GetLobby() ? GTFGCClientSystem()->GetLobby()->Obj().initial_average_mm_rating() : 0.f;
#endif

		// Compute the actual rating
		const int nNumLevel = m_pProgressionDesc->GetNumLevels();
		const uint32 nStart = m_pProgressionDesc->GetLevelByNumber( 0 ).m_nStartXP;
		const uint32 nRange = m_pProgressionDesc->GetLevelByNumber( nNumLevel ).m_nEndXP - nStart;
		const uint32 nRating = ( flNormalizedRating * nRange ) + nStart;

		// This is the first badge that's gold and the upper ~1/4 of ranks.  This is close enough
		// to the previous high-rank cutoff and offers and clear association between badge visuals
		// and the doors, and being "high rank".
		const LevelInfo_t& highRankLevel = m_pProgressionDesc->GetLevelByNumber( 10 );

		return nRating >= highRankLevel.m_nStartXP;
	}
#endif

#ifdef CLIENT_DLL
	virtual bool BGetRoundStartBannerParameters( int& nSkin, int& nBodyGroup ) const OVERRIDE
	{
		// The comp skins start at skin 8
		nSkin = 8 + TFGameRules()->GetRoundsPlayed();
		nBodyGroup = 1;
		return true;
	}

	virtual bool BGetRoundDoorParameters( int& nSkin, int& nLogoBodyGroup ) const OVERRIDE
	{
		nLogoBodyGroup = 0;
		nSkin = 0;
		if( BMatchIsHighSkill() )
		{
			// High skill has a different skin
			nSkin = 2;
		}

		return true;
	}
	virtual const char *GetMapLoadBackgroundOverride( bool bWidescreen ) const OVERRIDE
	{
		return ( bWidescreen ? "ranked_background_widescreen" : "ranked_background" );
	}
#endif

#ifdef GAME_DLL

	virtual void PostMatchClearServerSettings() const OVERRIDE
	{
		Assert( TFGameRules() );
		TFGameRules()->EndCompetitiveMatch();
	}

	virtual void InitGameRulesSettings() const OVERRIDE
	{
		TFGameRules()->SetCompetitiveMode( true );
		if ( !TFGameRules()->IsCommunityGameMode() )
		{
			TFGameRules()->SetAllowBetweenRounds( true );
		}
	}

	virtual void InitGameRulesSettingsPostEntity() const OVERRIDE
	{
		CTeamControlPointMaster *pMaster = ( g_hControlPointMasters.Count() ) ? g_hControlPointMasters[0] : NULL;
		bool bMultiStagePLR = ( tf_gamemode_payload.GetBool() && pMaster && pMaster->PlayingMiniRounds() && TFGameRules()->HasMultipleTrains() );
		bool bUseStopWatch = TFGameRules()->MatchmakingShouldUseStopwatchMode();
		bool bCTF = tf_gamemode_ctf.GetBool();
		bool bHighSkill = BMatchIsHighSkill();

		// Exec our match settings
		const char *pszExecFile = ( bHighSkill ) ? "server_competitive_rounds_win_conditions_high_skill.cfg" : "server_competitive_rounds_win_conditions.cfg";
		
		if ( bUseStopWatch )
		{
			pszExecFile = ( bHighSkill ) ? "server_competitive_stopwatch_win_conditions_high_skill.cfg" : "server_competitive_stopwatch_win_conditions.cfg";
		}
		else if ( bMultiStagePLR || bCTF )
		{
			pszExecFile = ( bHighSkill ) ? "server_competitive_max_rounds_win_conditions_high_skill.cfg" : "server_competitive_max_rounds_win_conditions.cfg";
		}

		engine->ServerCommand( CFmtStr( "exec %s\n", pszExecFile ) );

		TFGameRules()->SetInStopWatch( bUseStopWatch );
		mp_tournament_stopwatch.SetValue( bUseStopWatch );
	}

	bool ShouldRequestLateJoin() const OVERRIDE
	{
		auto pTFGameRules = TFGameRules();
		if ( !pTFGameRules || !pTFGameRules->IsCompetitiveMode() || pTFGameRules->IsManagedMatchEnded() )
		{
			return false;
		}

		const CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();

		int nPlayers = pMatch->GetNumActiveMatchPlayers();
		int nMissingPlayers = pMatch->GetCanonicalMatchSize() - nPlayers;

		// Allow late-join if we're not started yet, have missing players, and have not lost everyone.
		return nMissingPlayers && nPlayers &&
				(	pTFGameRules->State_Get() == GR_STATE_BETWEEN_RNDS ||
					pTFGameRules->State_Get() == GR_STATE_PREGAME ||
					pTFGameRules->State_Get() == GR_STATE_STARTGAME );
	}

	bool BMatchIsSafeToLeaveForPlayer( const CMatchInfo* pMatchInfo, const CMatchInfo::PlayerMatchData_t *pMatchPlayer ) const OVERRIDE
	{
		// It's only safe if the match is over
		return pMatchInfo->BMatchTerminated();
	}

	virtual bool BPlayWinMusic( int nWinningTeam, bool bGameOver ) const OVERRIDE
	{
		if ( bGameOver )
		{
			TFGameRules()->BroadcastSound( 255, ( nWinningTeam == TF_TEAM_RED ) ? "Announcer.CompMatchWinRed" : "Announcer.CompMatchWinBlu"  );
			TFGameRules()->BroadcastSound( 255, ( nWinningTeam == TF_TEAM_RED ) ? "MatchMaking.MatchEndRedWinMusic" : "MatchMaking.MatchEndBlueWinMusic"  );
		}
		else
		{
			if ( nWinningTeam == TF_TEAM_RED )
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.CompRoundWinRed" );
				TFGameRules()->BroadcastSound( 255, "MatchMaking.RoundEndRedWinMusic" );
			}
			else if ( nWinningTeam == TF_TEAM_BLUE )
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.CompRoundWinBlu" );
				TFGameRules()->BroadcastSound( 255, "MatchMaking.RoundEndBlueWinMusic" );
			}
			else
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.CompRoundStalemate" );
				TFGameRules()->BroadcastSound( 255, "MatchMaking.RoundEndStalemateMusic" );
			}
		}

		return true;
	}
#endif

	protected:


};

REGISTER_MATCH_GROUP_TYPE( k_eTFMatchGroup_Ladder_6v6, new CLadderMatchGroupDescription( k_eTFMatchGroup_Ladder_6v6, &tf_mm_match_size_ladder_6v6 ) );

class CEventPlaceholderMatchGroupDescription : public CLadderMatchGroupDescription
{
public:
	CEventPlaceholderMatchGroupDescription( ETFMatchGroup eMatchGroup, ConVar* pmm_match_group_size )
		:	CLadderMatchGroupDescription( eMatchGroup, pmm_match_group_size )
	{
		m_pProgressionDesc = GetProgressionDesc( k_eProgression_Glicko );

		m_pszModeNameLocToken		= "#TF_Matchmaking_SpecialEventPlaceholder";
		m_pszRichPresenceLocToken	= "SpecialEvent";
		m_bUseMatchSummaryStage		= false;
		m_nNumWinsToExitPlacement	= 5;
		m_eCurrentDisplayRating		= k_nMMRating_Comp_12v12_GLICKO;
		m_eLastAckdDisplayRating	= k_nMMRating_Comp_12v12_GLICKO_PlayerAcknowledged;
		m_eCurrentDisplayRank		= k_nMMRating_Comp_12v12_Rank;
		m_eLastAckdDisplayRank		= k_nMMRating_Comp_12v12_Rank_PlayerAcknowledged;
		// No leaderboard for these right meow
		m_eLeaderboard				= k_eMatchGroupLeaderboard_Invalid;

	}


#ifdef CLIENT_DLL
	virtual const PlayListData_t* GetPlayListEntryData() const
	{
		static PlayListData_t s_eventComp12v12PlayListData =
		{
			"#MMenu_PlayList_SpecialEventPlaceholder_Button",
			"#MMenu_PlayList_SpecialEventPlaceholder_Desc",
			"main_menu/main_menu_button_event",
			"#TF_Matchmaking_HeaderEventPlaceholder",
			"#TF_Matchmaking_DescEventPlaceholder",
			"main_menu/main_menu_button_event"
		};

		return &s_eventComp12v12PlayListData;
	}
#endif
};

REGISTER_MATCH_GROUP_TYPE( k_eTFMatchGroup_Event_Placeholder, new CEventPlaceholderMatchGroupDescription( k_eTFMatchGroup_Event_Placeholder, &tf_mm_match_size_ladder_12v12 ) );
