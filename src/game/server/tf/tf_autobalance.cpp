//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "tf_autobalance.h"
#include "tf_gamerules.h"
#include "tf_matchmaking_shared.h"
#include "team.h"
#include "minigames/tf_duel.h"
#include "player_resource.h"
#include "tf_player_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern ConVar mp_developer;
extern ConVar mp_teams_unbalance_limit;
extern ConVar tf_arena_use_queue;
extern ConVar mp_autoteambalance;
extern ConVar tf_autobalance_ask_candidates_maxtime;
extern ConVar tf_autobalance_dead_candidates_maxtime;
extern ConVar tf_autobalance_force_candidates_maxtime;
extern ConVar tf_autobalance_xp_bonus;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAutobalance::CTFAutobalance()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAutobalance::~CTFAutobalance()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::Reset()
{
	m_eCurrentState = AB_STATE_INACTIVE;
	m_iLightestTeam = m_iHeaviestTeam = TEAM_INVALID;
	m_nNeeded = 0;
	m_flNextStateChange = -1.f;

	m_vecCandidates.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::Shutdown()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::LevelShutdownPostEntity()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutobalance::ShouldBeActive() const
{
	if ( !TFGameRules() )
		return false;

	if ( TFGameRules()->IsInTraining() || TFGameRules()->IsInItemTestingMode() )
		return false;

	if ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() )
		return false;

	if ( TFGameRules()->IsCommunityGameMode() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		return pMatchDesc->BUsesAutoBalance();
	}

	// outside of managed matches, we don't normally do any balancing for tournament mode
	if ( TFGameRules()->IsInTournamentMode() )
		return false;

	return ( mp_autoteambalance.GetInt() == 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutobalance::AreTeamsUnbalanced()
{
	if ( !TFGameRules() )
		return false;

	// don't bother switching teams if the round isn't running
	if ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
		return false;

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

	if ( TFGameRules()->ArePlayersInHell() )
		return false;

	if ( !IsOkayToBalancePlayers() )
		return false;

	int nDiffBetweenTeams = 0;
	m_iLightestTeam = m_iHeaviestTeam = TEAM_INVALID;
	m_nNeeded = 0;

	CMatchInfo *pMatch = GTFGCClientSystem()->GetLiveMatch();
	if ( pMatch )
	{
		int nNumTeamRed = pMatch->GetNumActiveMatchPlayersForTeam( TFGameRules()->GetGCTeamForGameTeam( TF_TEAM_RED ) );
		int nNumTeamBlue = pMatch->GetNumActiveMatchPlayersForTeam( TFGameRules()->GetGCTeamForGameTeam( TF_TEAM_BLUE ) );

		m_iLightestTeam = ( nNumTeamRed > nNumTeamBlue ) ? TF_TEAM_BLUE : TF_TEAM_RED;
		m_iHeaviestTeam = ( nNumTeamRed > nNumTeamBlue ) ? TF_TEAM_RED : TF_TEAM_BLUE;

		nDiffBetweenTeams = abs( nNumTeamRed - nNumTeamBlue );
	}
	else
	{
		int iMostPlayers = 0;
		int iLeastPlayers = MAX_PLAYERS_ARRAY_SAFE;
		int i = FIRST_GAME_TEAM;

		for ( CTeam *pTeam = GetGlobalTeam( i ); pTeam != NULL; pTeam = GetGlobalTeam( ++i ) )
		{
			int iNumPlayers = pTeam->GetNumPlayers();

			if ( iNumPlayers < iLeastPlayers )
			{
				iLeastPlayers = iNumPlayers;
				m_iLightestTeam = i;
			}

			if ( iNumPlayers > iMostPlayers )
			{
				iMostPlayers = iNumPlayers;
				m_iHeaviestTeam = i;
			}
		}

		nDiffBetweenTeams = ( iMostPlayers - iLeastPlayers );
	}

	if ( nDiffBetweenTeams > mp_teams_unbalance_limit.GetInt() ) 
	{
		m_nNeeded = ( nDiffBetweenTeams / 2 );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutobalance::IsAlreadyCandidate( CTFPlayer *pTFPlayer ) const
{
	FOR_EACH_VEC( m_vecCandidates, i )
	{
		if ( m_vecCandidates[i].hPlayer.Get() == pTFPlayer )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
double CTFAutobalance::GetTeamAutoBalanceScore( int nTeam ) const
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetLiveMatch();
	if ( pMatch && TFGameRules() )
	{
		return pMatch->GetTotalSkillRatingForTeam( TFGameRules()->GetGCTeamForGameTeam( nTeam ) );
	}

	int nTotalScore = 0;
	int nTeamScore = 0;
	CTFPlayerResource *pTFPlayerResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	CTeam *pTeam = GetGlobalTeam( nTeam );
	if ( pTFPlayerResource && pTeam )
	{
		// Tally up total score across everyone and for the specified team
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			if ( pPlayer->GetTeam() == pTeam )
			{
				nTeamScore = pTFPlayerResource->GetTotalScore( pPlayer->entindex() );
			}

			nTotalScore += pTFPlayerResource->GetTotalScore( pPlayer->entindex() );
		}
	}

	return (double)nTeamScore / (double)nTotalScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
double CTFAutobalance::GetPlayerAutoBalanceScore( CTFPlayer *pTFPlayer ) const
{
	if ( !pTFPlayer )
		return 0.0;

	CMatchInfo *pMatch = GTFGCClientSystem()->GetLiveMatch();
	if ( pMatch )
	{
		CSteamID steamID;
		pTFPlayer->GetSteamID( &steamID );

		if ( steamID.IsValid() )
		{
			const CMatchInfo::PlayerMatchData_t* pPlayerMatchData = pMatch->GetMatchDataForPlayer( steamID );
			if ( pPlayerMatchData )
			{
				return pPlayerMatchData->flNormalizedMMSkillRating;
			}
		}
	}

	int nPlayerScore = 0;
	int nTotalScore = 0;
	CTFPlayerResource *pTFPlayerResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	if ( pTFPlayerResource )
	{
		// Tally up total score across everyone and find the particular player's score
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			if ( pPlayer == pTFPlayer )
			{
				nPlayerScore = pTFPlayerResource->GetTotalScore( pPlayer->entindex() );
			}

			nTotalScore += pTFPlayerResource->GetTotalScore( pPlayer->entindex() );
		}
		
	}

	return (double)nPlayerScore / (double)nTotalScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutobalance::ValidateCandidates()
{
	FOR_EACH_VEC_BACK( m_vecCandidates, i )
	{
		CTFPlayer *pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if ( !pTFPlayer || !pTFPlayer->CanBeAutobalanced() )
		{
			m_vecCandidates.Remove( i );
		}
	}

	return ( m_vecCandidates.Count() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFAutobalance::FindNextCandidate()
{
	CTFPlayer *pRetVal = NULL;

	CUtlVector< CTFPlayer* > vecCandidates;
	CTeam *pTeam = GetGlobalTeam( m_iHeaviestTeam );
	if ( pTeam )
	{
		// loop through and get a list of possible candidates
		for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pTeam->GetPlayer( i ) );
			if ( pTFPlayer && !IsAlreadyCandidate( pTFPlayer ) && pTFPlayer->CanBeAutobalanced() )
			{
				vecCandidates.AddToTail( pTFPlayer );
			}
		}
	}

	// no need to go any further if there's only one candidate
	if ( vecCandidates.Count() == 1 )
	{
		pRetVal = vecCandidates[0];
	}
	else if ( vecCandidates.Count() > 1 )
	{
		double fTotalDiff = fabs( GetTeamAutoBalanceScore( m_iHeaviestTeam ) - GetTeamAutoBalanceScore( m_iLightestTeam ) );
		double fAverageNeeded = ( fTotalDiff / 2.0 ) / m_nNeeded;

		// now look for a player on the heaviest team with skill rating closest to that average
		float fClosest = FLT_MAX;
		FOR_EACH_VEC( vecCandidates, iIndex )
		{
			double fDiff = fabs( fAverageNeeded - GetPlayerAutoBalanceScore( vecCandidates[iIndex] ) );
			if ( fDiff < fClosest )
			{
				fClosest = fDiff;
				pRetVal = vecCandidates[iIndex];
			}
		}
	}

	return pRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutobalance::FindCandidates()
{
	if ( !AreTeamsUnbalanced() )
	{
		Reset();
		return false;
	}

	m_vecCandidates.Purge();

	int nMinToFind = 0;
	CTeam *pTeam = GetGlobalTeam( m_iHeaviestTeam );
	if ( pTeam )
	{
		nMinToFind = ( int ) ( pTeam->GetNumPlayers() * 0.4f ); // 40% of the team
	}

	int nTotal = Max( ( m_nNeeded * 2 ), nMinToFind );
	int nNumFound = 0;

	while ( nNumFound < nTotal )
	{
		CTFPlayer *pTFPlayer = FindNextCandidate();
		if ( pTFPlayer )
		{
			// the best candidates are towards the tail of the list so
			// we can use for_each_vec_back to remove entries later
			int iIndex = m_vecCandidates.AddToHead();
			m_vecCandidates[iIndex].hPlayer = pTFPlayer;
			m_vecCandidates[iIndex].bSentForceMessage = false;

			nNumFound++;
			continue;
		}

		break;
	}

	if ( nNumFound <= 0 )
	{
		// we couldn't find anyone
		Reset();
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::PlayerChangeTeam( CTFPlayer *pTFPlayer, bool bFullBonus )
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetLiveMatch();
	if ( pMatch )
	{
		CSteamID steamID;
		pTFPlayer->GetSteamID( &steamID );

		// We're going to give the switching player a bonus pool of XP. This should encourage
		// them to keep playing to earn what's in the pool, rather than just quit after getting
		// a big payout
		if ( !pMatch->BSentResult() )
		{
			pMatch->GiveXPBonus( steamID, CMsgTFXPSource_XPSourceType_SOURCE_AUTOBALANCE_BONUS, 1, bFullBonus ? tf_autobalance_xp_bonus.GetInt() : ( tf_autobalance_xp_bonus.GetInt() / 2 ) );
		}

		GTFGCClientSystem()->ChangeMatchPlayerTeam( steamID, TFGameRules()->GetGCTeamForGameTeam( m_iLightestTeam ) );
	}

	pTFPlayer->ChangeTeam( m_iLightestTeam, false, false, true );
	pTFPlayer->ForceRespawn();
	pTFPlayer->SetLastAutobalanceTime( gpGlobals->curtime );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
	if ( event )
	{
		event->SetInt( "player", pTFPlayer->entindex() );
		event->SetInt( "team", m_iLightestTeam );
		gameeventmanager->FireEvent( event );
	}

	// tell people that we've switched this player
	CReliableBroadcastRecipientFilter filter;
	filter.RemoveRecipient( pTFPlayer );
	UTIL_ClientPrintFilter( filter, HUD_PRINTTALK, "#game_player_was_team_balanced", pTFPlayer->GetPlayerName() );

	// let the player know what happened
	const char *pszTeam = ( m_iLightestTeam == TF_TEAM_RED ) ? "#TF_RedTeam_Name" : "#TF_BlueTeam_Name";
	ClientPrint( pTFPlayer, HUD_PRINTTALK, ( pMatch ? "#TF_Autobalance_TeamChangeDone_Match" : "#TF_Autobalance_TeamChangeDone" ), pszTeam, CFmtStr( "%d", ( int ) ( bFullBonus ? tf_autobalance_xp_bonus.GetInt() : ( tf_autobalance_xp_bonus.GetInt() / 2 ) ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::ForceDeadCandidates()
{
	if ( !AreTeamsUnbalanced() || !ValidateCandidates() )
	{
		Reset();
		return;
	}

	FOR_EACH_VEC_BACK( m_vecCandidates, i )
	{
		CTFPlayer *pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if ( !pTFPlayer->IsAlive() && ( pTFPlayer->GetObserverMode() > OBS_MODE_FREEZECAM ) )
		{
			PlayerChangeTeam( pTFPlayer, true );
			m_vecCandidates.Remove( i );

			if ( !AreTeamsUnbalanced() )
			{
				Reset();
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::ForceCandidatesSetup()
{
	if ( !AreTeamsUnbalanced() || !ValidateCandidates() )
	{
		Reset();
		return;
	}

	// pick the number of players we need to switch and let them know it's going to happen
	const char *pszTeam = ( m_iLightestTeam == TF_TEAM_RED ) ? "#TF_RedTeam_Name" : "#TF_BlueTeam_Name";

	int nNumTold = 0;
	FOR_EACH_VEC_BACK( m_vecCandidates, i )
	{
		CTFPlayer *pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if ( pTFPlayer && ( nNumTold < m_nNeeded ) )
		{
			ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Autobalance_TeamChangePending", pszTeam, CFmtStr( "%d", tf_autobalance_force_candidates_maxtime.GetInt() ) );
			m_vecCandidates[i].bSentForceMessage = true;
			nNumTold++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::ForceCandidatesExecution()
{
	if ( !AreTeamsUnbalanced() || !ValidateCandidates() )
	{
		Reset();
		return;
	}

	int nNumSwitched = 0;
	FOR_EACH_VEC_BACK( m_vecCandidates, i )
	{
		// did we warn this player?
		CTFPlayer *pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if ( pTFPlayer && m_vecCandidates[i].bSentForceMessage )
		{
			// do we still need to switch players?
			if ( nNumSwitched < m_nNeeded )
			{
				PlayerChangeTeam( pTFPlayer, true );
				nNumSwitched++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutobalance::FrameUpdatePostEntityThink()
{
	if ( !ShouldBeActive() )
	{
		Reset();
		return;
	}

	switch ( m_eCurrentState )
	{
	case AB_STATE_INACTIVE:
		// we should be active if we've made it this far
		m_eCurrentState = AB_STATE_MONITOR;
		m_flNextStateChange = -1.f;
		break;
	case AB_STATE_MONITOR:
		if ( ( m_flNextStateChange > 0 ) && ( m_flNextStateChange < gpGlobals->curtime ) )
		{
			if ( FindCandidates() )
			{
				m_eCurrentState = AB_STATE_FORCE_DEAD_CANDIDATES;
				m_flNextStateChange = -1.f;
			}
		}
		else
		{
			if ( AreTeamsUnbalanced() )
			{
				if ( m_flNextStateChange < 0 )
				{
					m_flNextStateChange = gpGlobals->curtime;
				}
			}
			else
			{
				m_flNextStateChange = -1.f;
			}
		}
		break;
	case AB_STATE_FORCE_DEAD_CANDIDATES:
		if ( ( m_flNextStateChange > 0 ) && ( m_flNextStateChange < gpGlobals->curtime ) )
		{
			m_eCurrentState = AB_STATE_FORCE_CANDIDATES_SETUP;
			m_flNextStateChange = -1.f;
		}
		else
		{
			if ( m_flNextStateChange < 0 )
			{
				m_flNextStateChange = gpGlobals->curtime + tf_autobalance_dead_candidates_maxtime.GetFloat();
			}
			ForceDeadCandidates();
		}
		break;
	case AB_STATE_FORCE_CANDIDATES_SETUP:
		if ( ( m_flNextStateChange > 0 ) && ( m_flNextStateChange < gpGlobals->curtime ) )
		{
			m_eCurrentState = AB_STATE_FORCE_CANDIDATES_EXECUTION;
			m_flNextStateChange = -1.f;
		}
		else
		{
			if ( m_flNextStateChange < 0 )
			{
				m_flNextStateChange = gpGlobals->curtime + tf_autobalance_force_candidates_maxtime.GetFloat();
				ForceCandidatesSetup();
			}
		}
		break;
	case AB_STATE_FORCE_CANDIDATES_EXECUTION:
		ForceCandidatesExecution();
		Reset();
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutobalance::IsOkayToBalancePlayers()
{
	if ( GTFGCClientSystem()->GetLiveMatch() && !GTFGCClientSystem()->CanChangeMatchPlayerTeams() ) 
		return false;

	return true;
}

CTFAutobalance gTFAutobalance;
CTFAutobalance *TFAutoBalance(){ return &gTFAutobalance; }
