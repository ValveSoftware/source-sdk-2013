//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_match_description_shared.h"
#include "tf_progression_description.h"
#include "tf_matchmaking_shared.h"

#if   GAME_DLL
	#include "tf_gamerules.h"
#elif CLIENT_DLL
#endif

class CMvMMatchGroupDescription : public IMatchGroupDescription
{
	public:
	CMvMMatchGroupDescription( ETFMatchGroup eMatchGroup,
	                           const char* pszConfig,
	                           bool bTrustedOnly,
	                           const char* pszLocName,
	                           const char* pszRichPresenceLocToken,
	                           bool bStrictAbandons )
		: IMatchGroupDescription( eMatchGroup )
	{
		// MvM settings
		m_bActive						= true;
		m_eLateJoinMode 				= eMatchMode_MatchMaker_LateJoinDropIn;
		m_ePenaltyPool					= eMMPenaltyPool_Casual;
		m_pszExecFileName 				= pszConfig;
		m_pmm_match_group_size 			= &tf_mm_match_size_mvm;
		m_bShowPreRoundDoors 			= false;
		m_bShowPostRoundDoors 			= false;
		m_bRequireCompleteMatch 		= false;
		m_bTrustedServersOnly 			= bTrustedOnly;
		m_bAllowSpecModeChange			= true;
		m_bUsesDashboardOnRoundEnd		= false;
		m_pszModeNameLocToken			= pszLocName;
		m_pszRichPresenceLocToken		= pszRichPresenceLocToken;
		m_eMatchType					= MATCH_TYPE_MVM;
		m_pmm_required_score			= MVM_REQUIRED_SCORE;
		m_bUsesStrictAbandons			= bStrictAbandons;
	}


#ifdef CLIENT_DLL
	virtual bool BGetRoundStartBannerParameters( int& nSkin, int& nBodyGroup ) const OVERRIDE
	{
		// Dont show in MvM...for now
		return false;
	}

	virtual bool BGetRoundDoorParameters( int& nSkin, int& nLogoBodyGroup ) const OVERRIDE
	{
		// Don't show in MvM...for now
		return false;
	}

	virtual const char *GetMapLoadBackgroundOverride( bool bWidescreen ) const OVERRIDE
	{
		if ( bWidescreen )
		{
			return NULL;
		}

		return "mvm_background_map";
	}
#endif

#ifdef GAME_DLL
	virtual bool InitServerSettingsForMatch( const CTFGSLobby* pLobby ) const OVERRIDE
	{
		bool bRet = IMatchGroupDescription::InitServerSettingsForMatch( pLobby );

		if ( *pLobby->GetMissionName() != '\0' )
		{
			TFGameRules()->SetNextMvMPopfile( pLobby->GetMissionName() );
		}

		return bRet;
	}

	virtual void PostMatchClearServerSettings() const OVERRIDE
	{

	}

	virtual void InitGameRulesSettings() const OVERRIDE
	{
	}

	virtual void InitGameRulesSettingsPostEntity() const OVERRIDE
	{
	}

	bool ShouldRequestLateJoin() const OVERRIDE
	{
		if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
			return false;

		// Check game state
		switch ( TFGameRules()->State_Get() )
		{
		case GR_STATE_INIT:
		case GR_STATE_PREGAME:
		case GR_STATE_STARTGAME:
		case GR_STATE_PREROUND:
		case GR_STATE_TEAM_WIN:
		case GR_STATE_RESTART:
		case GR_STATE_STALEMATE:
		case GR_STATE_BONUS:
		case GR_STATE_BETWEEN_RNDS:
			return true;

		case GR_STATE_RND_RUNNING:
			if ( TFObjectiveResource() &&
			     !TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() &&
			     TFObjectiveResource()->GetMannVsMachineWaveCount() == TFObjectiveResource()->GetMannVsMachineMaxWaveCount() )
			{
				int nMaxEnemyCountNoSupport = TFObjectiveResource()->GetMannVsMachineWaveEnemyCount();
				if ( nMaxEnemyCountNoSupport <= 0 )
				{
					Assert( false ); // no enemies in wave?!
					return false;
				}

				// calculate number of remaining enemies
				int nNumEnemyRemaining = 0;

				for ( int i = 0; i < MVM_CLASS_TYPES_PER_WAVE_MAX_NEW; ++i )
				{
					int nClassCount = TFObjectiveResource()->GetMannVsMachineWaveClassCount( i );
					unsigned int iFlags = TFObjectiveResource()->GetMannVsMachineWaveClassFlags( i );

					if ( iFlags & MVM_CLASS_FLAG_MINIBOSS )
					{
						nNumEnemyRemaining += nClassCount;
					}

					if ( iFlags & MVM_CLASS_FLAG_NORMAL )
					{
						nNumEnemyRemaining += nClassCount;
					}
				}

				// if less then 40% of the last wave remains, lock people out from MM
				if ( (float)nNumEnemyRemaining / (float)nMaxEnemyCountNoSupport < 0.4f )
					return false;
			}
			return true;

		case GR_STATE_GAME_OVER:
			return false;
		}

		Assert( false );
		return false;
	}

	bool BMatchIsSafeToLeaveForPlayer( const CMatchInfo* pMatchInfo, const CMatchInfo::PlayerMatchData_t *pMatchPlayer ) const
	{
		bool bSafe = false;
		// Allow safe leaving after you have played for N seconds or if the match drops below N players, even if it is
		// still active.
		int nAllowAfterSeconds = tf_mvm_allow_abandon_after_seconds.GetInt();
		int nAllowBelowPlayers = tf_mvm_allow_abandon_below_players.GetInt();
		RTime32 now = CRTime::RTime32TimeCur();
		bSafe = bSafe || ( nAllowAfterSeconds > 0 && (uint32)nAllowAfterSeconds < ( now - pMatchPlayer->rtJoinedMatch ) );
		bSafe = bSafe || ( nAllowBelowPlayers > 0 && pMatchInfo->GetNumActiveMatchPlayers() < nAllowBelowPlayers );

		// Bootcamp is a magical nevar-abandon land
		bSafe = bSafe || ( m_eMatchGroup == k_eTFMatchGroup_MvM_Practice );

		return bSafe;
	}

	virtual bool BPlayWinMusic( int nWinningTeam, bool bGameOver ) const OVERRIDE
	{
		// Not handled
		return false;
	}
#endif
};

REGISTER_MATCH_GROUP_TYPE( k_eTFMatchGroup_MvM_Practice,
                           new CMvMMatchGroupDescription( k_eTFMatchGroup_MvM_Practice, "server_bootcamp.cfg",
                                                          /* bTrustedOnly */ false, "#TF_MvM_BootCamp", "BootCamp",
                                                          /* bStrictAbandons */ false ) );
REGISTER_MATCH_GROUP_TYPE( k_eTFMatchGroup_MvM_MannUp,
                           new CMvMMatchGroupDescription( k_eTFMatchGroup_MvM_Practice, "server_mannup.cfg",
                                                          /* bTrustedOnly */ true, "#TF_MvM_MannUp", "MannUp",
                                                          /* bStrictAbandons */ true ) );
