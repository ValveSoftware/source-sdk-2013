//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_match_description_shared.h"
#include "tf_progression_description.h"
#include "tf_progression.h"

#if   defined GAME_DLL
	#include "tf_gamerules.h"
	#include "tf_rating_data.h"
#elif defined CLIENT_DLL
	#include "tf_gamerules.h"
	#include "tf_ladder_data.h"
	#include "tf_rating_data.h"
#endif


class CCasualMatchGroupDescription : public IMatchGroupDescription
{
public:
	CCasualMatchGroupDescription( ETFMatchGroup eMatchGroup,
								  ConVar* pmm_match_group_size,
								  ConVar* pmm_match_group_size_minimum,
								  EProgressionDesc eProgression )
		: IMatchGroupDescription( eMatchGroup )
	{
		m_pProgressionDesc = GetProgressionDesc( eProgression );

		// Casual settings
		m_bActive						= true;
		m_eLateJoinMode					= eMatchMode_MatchMaker_LateJoinMatchBased;
		m_ePenaltyPool					= eMMPenaltyPool_Casual;
		m_bUsesSkillRatings				= true;
		m_bSupportsLowPriorityQueue		= true;
		m_bUseMatchHud					= true;
		m_pszExecFileName				= "server_casual.cfg";
		m_pmm_match_group_size			= pmm_match_group_size;
		m_pmm_match_group_size_minimum	= pmm_match_group_size_minimum;
		m_pszMatchEndKickWarning		= "#TF_Competitive_GameOver";
		m_pszMatchStartSound			= "MatchMaking.RoundStartCasual";
		m_bAutoReady					= true;
		m_bUseAutoBalance				= true;
		m_bRequireCompleteMatch			= false;
		m_bAllowSpecModeChange			= true;
		m_bAutomaticallyRequeueAfterMatchEnds	= true;
		m_bUsesMapVoteOnRoundEnd		= true;
		m_bScramblesTeamsOnRollingMatch = true;
		m_bUsesXP						= true;
		m_bUsesSurveys					= true;
		m_pszModeNameLocToken			= "#TF_Matchmaking_HeaderCasual";
		m_pszRichPresenceLocToken		= "Casual";
		m_bAllowPartyJoins				= true;
		m_bContractProgressAllowed		= true;
		m_eMatchType					= MATCH_TYPE_CASUAL;
		m_eCurrentDisplayRating			= k_nMMRating_Casual_XP;
		m_eLastAckdDisplayRating		= k_nMMRating_Casual_XP_PlayerAcknowledged;
		m_eCurrentDisplayRank			= k_nMMRating_Casual_12v12_Rank;
		m_eLastAckdDisplayRank			= k_nMMRating_Casual_12v12_Rank_PlayerAcknowledged;
		m_eLeaderboard					= k_eMatchGroupLeaderboard_Casual12v12;

		// Steam stats for this group. Tracking these for Steam Summer Sale 2019, may be unused after that?
	}


#ifdef CLIENT_DLL
	virtual bool BGetRoundStartBannerParameters( int& nSkin, int& nBodyGroup ) const OVERRIDE
	{
		nBodyGroup = 0;
		nSkin = TFGameRules()->GetRoundsPlayed();
		return true;
	}

	virtual bool BGetRoundDoorParameters( int& nSkin, int& nLogoBodyGroup ) const OVERRIDE
	{
		nLogoBodyGroup = 1;
		nSkin = 3;
		return true;
	}

	virtual const char *GetMapLoadBackgroundOverride( bool bWidescreen ) const OVERRIDE
	{
		return NULL;
	}
#endif

#ifdef GAME_DLL
	virtual void PostMatchClearServerSettings() const OVERRIDE
	{
		Assert( TFGameRules() );
		TFGameRules()->MatchSummaryEnd();
	}

	virtual void InitGameRulesSettings() const OVERRIDE
	{
		if ( !TFGameRules()->IsCommunityGameMode() )
			TFGameRules()->SetAllowBetweenRounds( true );
	}

	virtual void InitGameRulesSettingsPostEntity() const OVERRIDE
	{
		CTeamControlPointMaster *pMaster = ( g_hControlPointMasters.Count() ) ? g_hControlPointMasters[0] : NULL;
		bool bMultiStagePLR = ( tf_gamemode_payload.GetBool() && pMaster && pMaster->PlayingMiniRounds() && 
								pMaster->GetNumRounds() > 1 && TFGameRules()->HasMultipleTrains() );
		bool bCTF = tf_gamemode_ctf.GetBool();
		bool bUseStopWatch = TFGameRules()->MatchmakingShouldUseStopwatchMode();

		// Exec our match settings
		const char *pszExecFile = bUseStopWatch ? "server_casual_stopwatch_win_conditions.cfg" : 
								  ( ( bMultiStagePLR || bCTF ) ? "server_casual_max_rounds_win_conditions.cfg" : "server_casual_rounds_win_conditions.cfg" );

		if ( TFGameRules()->IsPowerupMode() )
		{
			pszExecFile = "server_casual_max_rounds_win_conditions_mannpower.cfg";
		}

		if ( TFGameRules()->IsCommunityGameMode() )
		{
			pszExecFile = "server_custom.cfg";
		}

		char szCurrentMap[ MAX_MAP_NAME ];
		Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

		if ( !Q_stricmp( szCurrentMap, "arena_lumberyard_event" ) )
		{
			pszExecFile = "server_casual_max_rounds_win_conditions_custom.cfg";
		}

		engine->ServerCommand( CFmtStr( "exec %s\n", pszExecFile ) );

		// leave stopwatch off for now
		TFGameRules()->SetInStopWatch( false );//bUseStopWatch );
		mp_tournament_stopwatch.SetValue( false );//bUseStopWatch );

		// Hack for now, this map is having issues with players getting dropped while connecting
		if ( !Q_stricmp( szCurrentMap, "pl_corruption" ) )
		{
			engine->ServerCommand( "exec server_net_chan_extend.cfg\n" );
		}
	}

	bool ShouldRequestLateJoin() const OVERRIDE
	{
		auto pTFGameRules = TFGameRules();
		if ( !pTFGameRules || !pTFGameRules->IsCompetitiveMode() || pTFGameRules->IsManagedMatchEnded() )
		{
			Assert( false );
			return false;
		}

		if ( pTFGameRules->BIsManagedMatchEndImminent() )
			return false;

		const CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();

		int nPlayers = pMatch->GetNumActiveMatchPlayers();
		int nMissingPlayers = pMatch->GetCanonicalMatchSize() - nPlayers;

		// Allow late-join if we have missing players, have not lost everyone
		return nMissingPlayers && nPlayers;
	}

	bool BMatchIsSafeToLeaveForPlayer( const CMatchInfo* pMatchInfo, const CMatchInfo::PlayerMatchData_t *pMatchPlayer ) const
	{
		return true;
	}

	virtual bool BPlayWinMusic( int nWinningTeam, bool bGameOver ) const OVERRIDE
	{
		// Custom for game over
		if ( bGameOver )
		{
			TFGameRules()->BroadcastSound( TF_TEAM_RED, nWinningTeam == TF_TEAM_RED ? "MatchMaking.MatchEndWinMusicCasual" : "MatchMaking.MatchEndLoseMusicCasual" );
			TFGameRules()->BroadcastSound( TF_TEAM_BLUE, nWinningTeam == TF_TEAM_BLUE ? "MatchMaking.MatchEndWinMusicCasual" : "MatchMaking.MatchEndLoseMusicCasual" );
			return true;
		}
		else
		{
			// Let non-match logic handle round wins
			return false;
		}
	}
#endif
};

REGISTER_MATCH_GROUP_TYPE( k_eTFMatchGroup_Casual_12v12, new CCasualMatchGroupDescription( k_eTFMatchGroup_Casual_12v12, &tf_mm_match_size_ladder_12v12, &tf_mm_match_size_ladder_12v12_minimum, k_eProgression_Casual ) );

