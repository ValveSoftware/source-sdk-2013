//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"
#include "econ_wearable.h"
#include "achievements_tf.h"
#include "usermessages.h"

// NVNT include for tf2 damage
#include "haptics/haptic_utils.h"

CAchievementMgr g_AchievementMgrTF;	// global achievement mgr for TF

bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole );

// Grace period that we allow a player to start after level init and still consider them to be participating for the full round.  This is fairly generous
// because it can in some cases take a client several minutes to connect with respect to when the server considers the game underway
#define TF_FULL_ROUND_GRACE_PERIOD	( 4 * 60.0f )

bool IsLocalTFPlayerClass( int iClass );


bool CBaseTFAchievementSimple::LocalPlayerCanEarn( void ) 
{ 
	if ( TFGameRules() )
	{
		bool bMVMAchievement = ( m_iAchievementID >= ACHIEVEMENT_TF_MVM_START_RANGE && m_iAchievementID <= ACHIEVEMENT_TF_MVM_END_RANGE );

		if ( bMVMAchievement )
		{
			if ( !TFGameRules()->IsMannVsMachineMode() || ( GetLocalPlayerTeam() != TF_TEAM_PVE_DEFENDERS ) )
			{
				return false;
			}
		}
		else
		{
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				return false;
			}
		}
	}

	return BaseClass::LocalPlayerCanEarn();
}

void CBaseTFAchievementSimple::FireGameEvent( IGameEvent *event )
{
	if ( !LocalPlayerCanEarn() )
		return;

	BaseClass::FireGameEvent( event );
}


bool CBaseTFAchievement::LocalPlayerCanEarn( void ) 
{ 
	// Swallow game events if we're not allowed to earn achievements, or if the local player isn't the right class
	if ( !GameRulesAllowsAchievements() )
	{
		return false;
	}

	// Determine class & check it
	if ( m_iAchievementID >= ACHIEVEMENT_START_CLASS_SPECIFIC && m_iAchievementID <= ACHIEVEMENT_END_CLASS_SPECIFIC )
	{
		int iClass = floor( (m_iAchievementID - ACHIEVEMENT_START_CLASS_SPECIFIC) / 100.0f ) + 1;
		if ( !IsLocalTFPlayerClass( iClass ) )
		{
			return false;
		}
	}

	return BaseClass::LocalPlayerCanEarn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAchievementFullRound::Init() 
{
	m_iFlags |= ACH_FILTER_FULL_ROUND_ONLY;		
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAchievementFullRound::ListenForEvents()
{
	ListenForGameEvent( "teamplay_round_win" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAchievementFullRound::FireGameEvent_Internal( IGameEvent *event )
{
	if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			// is the player currently on a game team?
			int iTeam = pLocalPlayer->GetTeamNumber();
			if ( iTeam >= FIRST_GAME_TEAM ) 
			{
				float flRoundTime = event->GetFloat( "round_time", 0 );
				if ( flRoundTime > 0 )
				{
					Event_OnRoundComplete( flRoundTime, event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAchievementFullRound::PlayerWasInEntireRound( float flRoundTime )
{
	float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
	if ( flTeamplayStartTime > 0 ) 
	{	
		// has the player been present and on a game team since the start of this round (minus a grace period)?
		if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAchievementTFPlayGameEveryClass : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTFPlayGameEveryClass, CTFAchievementFullRound );
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( ( TF_LAST_NORMAL_CLASS - 1 ) - TF_FIRST_NORMAL_CLASS + 1 ); //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
		BaseClass::Init();
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		float flLastClassChangeTime = m_pAchievementMgr->GetLastClassChangeTime();
		if ( flLastClassChangeTime > 0 ) 
		{					
			// has the player been present and not changed class since the start of this round (minus a grace period)?
			if ( flLastClassChangeTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			{
				C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pTFPlayer )
				{
					int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
					if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= ( TF_LAST_NORMAL_CLASS - 1 ) ) //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
					{
						// yes, the achievement is satisfied for this class, set the corresponding bit
						int iBitNumber =( iClass - TF_FIRST_NORMAL_CLASS );
						EnsureComponentBitSetAndEvaluate( iBitNumber );
					}							
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPlayGameEveryClass, ACHIEVEMENT_TF_PLAY_GAME_EVERYCLASS, "TF_PLAY_GAME_EVERYCLASS", 5 );

class CAchievementTFPlayGameEveryMap : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTFPlayGameEveryMap, CTFAchievementFullRound );
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_FILTER_FULL_ROUND_ONLY );

		static const char *szComponents[] =
		{
			"cp_dustbowl", "cp_granary", "cp_gravelpit", "cp_well", "ctf_2fort", "tc_hydro"
		};		
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetGoal( m_iNumComponents );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
		if ( flTeamplayStartTime > 0 ) 
		{	
			// has the player been present and on a game team since the start of this round (minus a grace period)?
			if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			{
				// yes, the achievement is satisfied for this map, set the corresponding bit
				OnComponentEvent( m_pAchievementMgr->GetMapName() );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPlayGameEveryMap, ACHIEVEMENT_TF_PLAY_GAME_EVERYMAP, "TF_PLAY_GAME_EVERYMAP", 5 );

class CAchievementTFGetHealPoints : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25000 );		
	}

	void OnPlayerStatsUpdate()
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[TFSTAT_HEALING];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFGetHealPoints, ACHIEVEMENT_TF_GET_HEALPOINTS, "TF_GET_HEALPOINTS", 5 );

class CAchievementTFBurnPlayersInMinimumTime : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFBurnPlayersInMinimumTime, ACHIEVEMENT_TF_BURN_PLAYERSINMINIMIMTIME, "TF_BURN_PLAYERSINMINIMUMTIME", 5 );

class CAchievementTFGetTurretKills : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFGetTurretKills, ACHIEVEMENT_TF_GET_TURRETKILLS, "TF_GET_TURRETKILLS", 5 );

class CAchievementTFGetHeadshots: public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 25 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// was this a headshot by this player?
		if ( ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) && ( IsHeadshot(event->GetInt( "customkill" ) ) ) )
		{
			// Increment count.  Count will also get slammed whenever we get a stats update from server.  They should generally agree,
			// but server is authoritative.
			IncrementCount();
		}
	}

	void OnPlayerStatsUpdate()
	{
		// when stats are updated by server, use most recent stat value
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_SNIPER );
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[TFSTAT_HEADSHOTS];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_SNIPER ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFGetHeadshots, ACHIEVEMENT_TF_GET_HEADSHOTS, "TF_GET_HEADSHOTS", 5 );

class CAchievementTFKillNemesis : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 5 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{		
		if ( ( ( event->GetInt( "death_flags" ) & TF_DEATH_REVENGE ) ) && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			// local player got revenge as primary killer
			IncrementCount();
		}
		else if ( event->GetInt( "death_flags" ) & TF_DEATH_ASSISTER_REVENGE )
		{
			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
			if ( iAssisterIndex > 0 )
			{
				CBaseEntity *pAssister = UTIL_PlayerByIndex( iAssisterIndex );
				if ( pAssister && ( pAssister == C_TFPlayer::GetLocalTFPlayer() ) )
				{
					// local player got revenge as assister
					IncrementCount();
				}
			}				
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFKillNemesis, ACHIEVEMENT_TF_KILL_NEMESIS, "TF_KILL_NEMESIS", 5 );

class CAchievementTFGetConsecutiveKillsNoDeaths : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iConsecutiveKills = 0;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer == pVictim )
		{
			m_iConsecutiveKills = 0;
		}
		else if ( pLocalPlayer == pAttacker )
		{
			m_iConsecutiveKills++;
			if ( 5 == m_iConsecutiveKills )
			{
				IncrementCount();
			}
		}	
	}
	int m_iConsecutiveKills;
};
DECLARE_ACHIEVEMENT( CAchievementTFGetConsecutiveKillsNoDeaths, ACHIEVEMENT_TF_GET_CONSECUTIVEKILLS_NODEATHS, "TF_GET_CONSECUTIVEKILLS_NODEATHS", 10 );

class CAchievementTFGetHealedByEnemy: public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFGetHealedByEnemy, ACHIEVEMENT_TF_GET_HEALED_BYENEMY, "TF_GET_HEALED_BYENEMY", 15 );

class CAchievementTFPlayGameFriendsOnly : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Are there at least 5 friends in the game?  (at least 8 players total)
			if ( CalcPlayersOnFriendsList( 5 ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPlayGameFriendsOnly, ACHIEVEMENT_TF_PLAY_GAME_FRIENDSONLY, "TF_PLAY_GAME_FRIENDSONLY", 10 );

class CAchievementTFWinMultipleGames : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTFWinMultipleGames, CTFAchievementFullRound );
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( 20 );
		BaseClass::Init();
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			// was the player on the winning team?
			int iPlayerTeam = pLocalPlayer->GetTeamNumber();
			int iWinningTeam = event->GetInt( "team" );
			if ( ( iWinningTeam >= FIRST_GAME_TEAM ) && ( iPlayerTeam == iWinningTeam ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWinMultipleGames, ACHIEVEMENT_TF_WIN_MULTIPLEGAMES, "TF_WIN_MULTIPLEGAMES", 10 );

class CAchievementTFGetMultipleKills : public CBaseTFAchievementSimple
{
	void Init() 
	{
		// listen for player kill enemy events, base class will increment count each time that happens
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1000 );
	}

	void OnPlayerStatsUpdate()
	{
		// when stats are updated by server, use most recent stat values

		int iKills = 0;
		// get sum of kills per class across all classes to get total kills
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= ( TF_LAST_NORMAL_CLASS - 1 ); iClass++ ) //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
		{
			ClassStats_t &classStats = CTFStatPanel::GetClassStats( iClass );
			iKills += classStats.accumulated.m_iStat[TFSTAT_KILLS];
		}

		int iOldCount = m_iCount;
		m_iCount = iKills;
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		EvaluateNewAchievement();
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFGetMultipleKills, ACHIEVEMENT_TF_GET_MULTIPLEKILLS, "TF_GET_MULTIPLEKILLS", 15 );

class CAchievementTFWin2FortNoEnemyCaps : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "ctf_2fort" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			if ( event->GetBool( "was_sudden_death" ) == false )
			{
				if ( event->GetInt( "team" ) == GetLocalPlayerTeam() )
				{
					// did the enemy team get any flag captures?
					C_TFTeam *pEnemyTeam = GetGlobalTFTeam( TF_TEAM_BLUE + TF_TEAM_RED - GetLocalPlayerTeam() );
					if ( 0 == pEnemyTeam->GetFlagCaptures() )
					{										
						IncrementCount();
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWin2FortNoEnemyCaps, ACHIEVEMENT_TF_WIN_2FORT_NOENEMYCAPS, "TF_WIN_2FORT_NOENEMYCAPS", 5 );

class CAchievementTFWinWellMinimumTime : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_well" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			if ( event->GetInt( "team" ) == GetLocalPlayerTeam() )
			{
				float flRoundTime = event->GetFloat( "round_time", 0 );
				if ( flRoundTime > 0 && flRoundTime < 5 * 60 )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWinWellMinimumTime, ACHIEVEMENT_TF_WIN_WELL_MINIMUMTIME, "TF_WIN_WELL_MINIMUMTIME", 10 );

class CAchievementTFWinHydroNoEnemyCaps : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( 1 );
		SetMapNameFilter( "tc_hydro" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// winning hydro with no enemy caps means there were 2 previous minirounds completed (3 total for a shutout)
		// and local player's team won the final round
		if ( ( 2 == m_pAchievementMgr->GetMiniroundsCompleted() ) && ( CheckWinNoEnemyCaps( event, TEAM_ROLE_NONE ) ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWinHydroNoEnemyCaps, ACHIEVEMENT_TF_WIN_HYDRO_NOENEMYCAPS, "TF_WIN_HYDRO_NOENEMYCAPS", 20 );

class CAchievementTFWinDustbowlNoEnemyCaps : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( 1 );
		SetMapNameFilter( "cp_dustbowl" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// defending dustbowl with no enemy caps means there were no previous minirounds completed (that would be an attacker capture),
		// the player was on the defending team and they won with no enemy caps
		if ( ( 0 == m_pAchievementMgr->GetMiniroundsCompleted() ) && CheckWinNoEnemyCaps( event, TEAM_ROLE_DEFENDERS ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWinDustbowlNoEnemyCaps, ACHIEVEMENT_TF_WIN_DUSTBOWL_NOENEMYCAPS, "TF_WIN_DUSTBOWL_NOENEMYCAPS", 10 );

class CAchievementTFWinGravelPitNoEnemyCaps : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_gravelpit" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// Was player on defenders and won with no enemy caps?
		if ( CheckWinNoEnemyCaps( event, TEAM_ROLE_DEFENDERS ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWinGravelPitNoEnemyCaps, ACHIEVEMENT_TF_WIN_GRAVELPIT_NOENEMYCAPS, "TF_WIN_GRAVELPIT_NOENEMYCAPS", 30 );

class CAchievementTFKillEnemiesAfterTeleporting : public CTFAchievementTeleporterTimingKills<CBaseAchievement>
{
	// stub -- all code in parent class
};
DECLARE_ACHIEVEMENT( CAchievementTFKillEnemiesAfterTeleporting, ACHIEVEMENT_TF_GENERAL_KILL_ENEMIES_AFTER_TELEPORTING, "TF_GENERAL_KILL_ENEMIES_AFTER_TELEPORTING", 10 );

//-----------------------------------------------------------------------------
// Purpose: see if a round win was a win for the local player with no enemy caps
//-----------------------------------------------------------------------------
bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole )
{
	if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
	{
		if ( event->GetInt( "team" ) == GetLocalPlayerTeam() )
		{
			int iLosingTeamCaps = event->GetInt( "losing_team_num_caps" );
			if ( 0 == iLosingTeamCaps )
			{
				C_TFTeam *pLocalTeam = GetGlobalTFTeam( GetLocalPlayerTeam() );
				if ( pLocalTeam )
				{
					int iRolePlayer = pLocalTeam->GetRole();
					if ( iRole > TEAM_ROLE_NONE && ( iRolePlayer != iRole ) )
						return false;
					return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to determine if local player is specified class
//-----------------------------------------------------------------------------
bool IsLocalTFPlayerClass( int iClass )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	return( pLocalPlayer && pLocalPlayer->IsPlayerClass( iClass ) );
}

//-----------------------------------------------------------------------------
// Purpose: Query if the gamerules allows achievement progress at this time
//-----------------------------------------------------------------------------
bool GameRulesAllowsAchievements( void )
{
	bool bRetVal = false;
	
	if ( ( TFGameRules()->State_Get() < GR_STATE_TEAM_WIN ) ||
		 ( TFGameRules()->State_Get() == GR_STATE_STALEMATE ) )
	{
		bRetVal = true;
	}

	return bRetVal;
}

//----------------------------------------------------------------------------------------------------------------
// Receive the PlayerIgnitedInv user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerIgnitedInv )
{
	int iPyroEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();
	int iMedicEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_ignited_inv" );
	if ( event )
	{
		event->SetInt( "pyro_entindex", iPyroEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );
		event->SetInt( "medic_entindex", iMedicEntIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}

// Receive the PlayerIgnited user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerIgnited )
{
	int iPyroEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();
	int iWeaponID = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_ignited" );
	if ( event )
	{
		event->SetInt( "pyro_entindex", iPyroEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );
		event->SetInt( "weaponid", iWeaponID );
		gameeventmanager->FireEventClientSide( event );
	}
}

// Receive the Damage user message and send out a clientside event for achievements to hook.
USER_MESSAGE( Damage )
{
	int iDamage = msg.ReadShort();
	int iDmgBits = msg.ReadLong();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_damaged" );
	if ( event )
	{
		event->SetInt( "amount", iDamage );
		event->SetInt( "type", iDmgBits );
		gameeventmanager->FireEventClientSide( event );
	}
	// NVNT START implementing rest of message for damage directions
	if(iDamage == 0)
		return; // no damage forces for no damage.
	
	// get the local player.
	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if(!pLocal)
		return;// if we dont have a local player ignore this message.

	Vector attackerPosition(0,0,0);
	if(msg.ReadOneBit())
	{
		// if we read one bit then that means we have shooters origin.
		msg.ReadBitVec3Coord( attackerPosition );
	}else{
		// if it is non origin, just set the origin below the player
		attackerPosition = pLocal->GetAbsOrigin() + Vector(0,0,-10);
	}
	// get the direction in world
	Vector attackDirectionLocal(vec3_origin);
	// rotate the direction to the local players view
	pLocal->WorldToEntitySpace(attackerPosition, &attackDirectionLocal);
	
	if ( haptics )
	{
		Vector hapticSpace( attackDirectionLocal.y, -attackDirectionLocal.z, attackDirectionLocal.x );

		hapticSpace.NormalizeInPlace();

		haptics->ApplyDamageEffect((float)iDamage, iDmgBits, hapticSpace);
	}
	// NVNT END
}

// Receive the UpdateAchievement user message and send out a clientside event for achievements to hook.
USER_MESSAGE( UpdateAchievement )
{
	int iIndex = (int) msg.ReadShort();
	int nData = (int) msg.ReadShort();

	g_AchievementMgrTF.UpdateAchievement( iIndex, nData );
}

// Receive the PlayerJarated user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerJarated )
{
	int iThrowerEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_jarated" );
	if ( event )
	{
		event->SetInt( "thrower_entindex", iThrowerEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}

USER_MESSAGE( PlayerJaratedFade )
{
	int iThrowerEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_jarated_fade" );
	if ( event )
	{
		event->SetInt( "thrower_entindex", iThrowerEntIndex );
		event->SetInt( "victim_entindex", iVictimEntIndex );

		gameeventmanager->FireEventClientSide( event );
	}
}
//This is so dumb.
USER_MESSAGE( PlayerShieldBlocked )
{
	int iAttacker = (int) msg.ReadByte();
	int iBlocker = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_shield_blocked" );
	if ( event )
	{
		event->SetInt( "attacker_entindex", iAttacker );
		event->SetInt( "blocker_entindex", iBlocker );

		gameeventmanager->FireEventClientSide( event );
	}
}

// Receive the PlayerExtinguished user message and send out a clientside event for achievements to hook.
USER_MESSAGE( PlayerExtinguished )
{
	int iMedicEntIndex = (int) msg.ReadByte();
	int iVictimEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_extinguished" );
	if ( event )
	{
		event->SetInt( "victim", iVictimEntIndex );
		event->SetInt( "healer", iMedicEntIndex );

		gameeventmanager->FireEventClientSide( event );
	}
}

void CAchievementTopScoreboard::Init() 
{
	SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
	SetGoal(1);
}

void CAchievementTopScoreboard::ListenForEvents()
{
	BaseClass::ListenForEvents();
	ListenForGameEvent( "teamplay_round_active" );
}

void CAchievementTopScoreboard::Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
{
	if ( PlayerWasInEntireRound( flRoundTime ) )
	{
		int iLocalTeam = C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber();

		if ( GetGlobalTFTeam(iLocalTeam) && GetGlobalTFTeam(iLocalTeam)->GetNumPlayers() >= 6 )
		{
			if ( g_TF_PR )
			{
				bool bHighest = true;
				int iLocalScore = g_TF_PR->GetTotalScore( C_TFPlayer::GetLocalTFPlayer()->entindex() );

				// See if the player's on the top of the scoreboard
				for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
				{
					if ( !g_PR->IsConnected( playerIndex ) || g_PR->IsLocalPlayer( playerIndex ) )
						continue;

					if ( g_PR->GetTeam(playerIndex) != iLocalTeam )
						continue;

					if ( g_TF_PR->GetTotalScore(playerIndex) > iLocalScore )
					{
						bHighest = false;
						break;
					}
				}

				if ( bHighest )
				{
					IncrementCount();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern int Training_GetNumCourses();
extern int Training_GetProgressCount();
class CAchievementTFCompleteTraining : public CBaseTFAchievementSimple
{
	DECLARE_CLASS( CAchievementTFCompleteTraining, CBaseAchievement );
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS );
		SetGoal( Training_GetNumCourses() );
		SetStoreProgressInSteam( true );
		SetCount( Training_GetProgressCount() );
		BaseClass::Init();
	}

	virtual void UpdateAchievement( int nData )
	{
		SetCount( nData );
		EvaluateNewAchievement();
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFCompleteTraining, ACHIEVEMENT_TF_COMPLETE_TRAINING, "TF_COMPLETE_TRAINING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_FireWaterJump : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_FireWaterJump, ACHIEVEMENT_TF_FIRE_WATERJUMP, "TF_FIRE_WATERJUMP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFChristmasCollectGifts : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "christmas_gift_grab" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
			return;

		if ( Q_strcmp( event->GetName(), "christmas_gift_grab" ) == 0 )
		{
			int iPlayer = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( iPlayer );

			if ( pPlayer && pPlayer == C_TFPlayer::GetLocalTFPlayer() )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFChristmasCollectGifts, ACHIEVEMENT_TF_CHRISTMAS_COLLECT_GIFTS, "TF_CHRISTMAS_COLLECT_GIFTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_KillBalloonicornOwners : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 79 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CSchemaItemDefHandle pItemDef_Balloonicorn( "Pet Balloonicorn" );
		CSchemaItemDefHandle pItemDef_BalloonicornPromo( "Pet Balloonicorn Promo" );
		CSchemaItemDefHandle pItemDef_BalloonicornPlushPromo( "Pet Balloonicorn Plush Promo" );
		CSchemaItemDefHandle pItemDef_Reindoonicorn( "Pet Reindoonicorn" );
		CSchemaItemDefHandle pItemDef_Balloonicorpse( "Balloonicorpse" );

		C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pLocalPlayer )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && ( pAttacker == pLocalPlayer ) && ( pTFVictim->GetTeamNumber() != pLocalPlayer->GetTeamNumber() ) )
			{
				int nVisionOptInFlags = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, nVisionOptInFlags, vision_opt_in_flags );

				// Does the local player have PyroVision on?
				if ( nVisionOptInFlags & TF_VISION_FILTER_PYRO )
				{
					// Is the victim wearing the Balloonicorn?
					for ( int i = 0 ; i < pTFVictim->GetNumWearables() ; ++i )
					{
						C_EconWearable *pWearable = pTFVictim->GetWearable( i );
						if ( pWearable && pWearable->GetAttributeContainer() )
						{
							CEconItemView *pItem = pWearable->GetAttributeContainer()->GetItem();
							if ( pItem && pItem->IsValid() )
							{
								if ( pItem->GetItemDefinition() == pItemDef_Balloonicorn ||
									 pItem->GetItemDefinition() == pItemDef_BalloonicornPromo ||
									 pItem->GetItemDefinition() == pItemDef_BalloonicornPlushPromo ||
									 pItem->GetItemDefinition() == pItemDef_Reindoonicorn ||
									 pItem->GetItemDefinition() == pItemDef_Balloonicorpse )
								{
									IncrementCount();
								}
							}
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_KillBalloonicornOwners, ACHIEVEMENT_TF_KILL_BALLOONICORN_OWNERS, "TF_KILL_BALLOONICORN_OWNERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MultipleBFF : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "remove_nemesis_relationships" );

		// clear data on level init
		m_hBFFs.Purge();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "remove_nemesis_relationships" ) )
		{
			int iPlayerIndex = event->GetInt( "player" );
			if ( iPlayerIndex == GetLocalPlayerIndex() )
			{
				// this is the local player, clear our list
				m_hBFFs.Purge();
			}
			else
			{
				// is this player in our list?
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );

				int index = m_hBFFs.Find( pPlayer );
				if ( index != m_hBFFs.InvalidIndex() )
				{
					m_hBFFs.Remove( index );
				}
			}
		}
	}

	void ValidateList( void )
	{
		// remove anyone on the local player's team to clean up changing teams, getting
		// team balanced, etc., if they're still on the other team, we're fine.
		for ( int i = m_hBFFs.Count() - 1 ; i >= 0 ; i-- )
		{
			C_TFPlayer *pPlayer = m_hBFFs[i];
			if ( pPlayer->GetTeamNumber() == GetLocalPlayerTeam() )
			{
				m_hBFFs.Remove( i );
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pLocalPlayer )
		{
			CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			int iDeathFlags = event->GetInt( "death_flags" );
			bool bDomination = ( iDeathFlags & TF_DEATH_DOMINATION ) || ( iDeathFlags & TF_DEATH_ASSISTER_DOMINATION );
			bool bRevenge = ( iDeathFlags & TF_DEATH_REVENGE ) || ( iDeathFlags & TF_DEATH_ASSISTER_REVENGE );

			if ( pTFVictim && ( pTFAttacker == pLocalPlayer ) && bDomination )
			{
				ValidateList();

				int nVisionOptInFlags = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, nVisionOptInFlags, vision_opt_in_flags );

				if ( nVisionOptInFlags & TF_VISION_FILTER_PYRO )
				{
					int index = m_hBFFs.Find( pTFVictim );
					if ( index == m_hBFFs.InvalidIndex() )
					{
						m_hBFFs.AddToHead( pTFVictim );
					}
				
					if ( m_hBFFs.Count() >= 2 )
					{
						IncrementCount();
					}
				}
			}
			else if ( pTFAttacker && ( pTFVictim == pLocalPlayer ) && bRevenge )
			{
				ValidateList();

				int index = m_hBFFs.Find( pTFAttacker );
				if ( index != m_hBFFs.InvalidIndex() )
				{
					m_hBFFs.Remove( index );
				}
			}
		}
	}

private:
	CUtlVector< CHandle<C_TFPlayer> > m_hBFFs;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MultipleBFF, ACHIEVEMENT_TF_MULTIPLE_BFF, "TF_MULTIPLE_BFF", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_TeamPyrovision : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		SetNextThink( 1.0 );
	}

	virtual void Think( void )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() >= FIRST_GAME_TEAM ) )
		{
			int nVisionOptInFlags = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, nVisionOptInFlags, vision_opt_in_flags );
			
			if ( nVisionOptInFlags & TF_VISION_FILTER_PYRO )
			{
				int nCount = 0;

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					C_BasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					if ( pPlayer == pLocalPlayer )
					{
						nCount++;
					}
					else if ( pPlayer->GetTeamNumber() == pLocalPlayer->GetTeamNumber() )
					{
						int nTempFlags = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nTempFlags, vision_opt_in_flags );

						if ( nTempFlags & TF_VISION_FILTER_PYRO )
						{
							nCount++;
						}
					}
				}

				if ( nCount >= 6 )
				{
					IncrementCount();
					ClearThink();
					return;
				}
			}
		}

		SetNextThink( 1.0 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_TeamPyrovision, ACHIEVEMENT_TF_TEAM_PYROVISION, "TF_TEAM_PYROVISION", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_DominateForGoggles : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		bool bDomination = event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION;

		if ( pTFVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) && ( bDomination == true ) )
		{
			// are they wearing the PyroVision goggles?
			for ( int i = 0 ; i < pTFVictim->GetNumWearables() ; ++i )
			{
				C_EconWearable *pWearable = pTFVictim->GetWearable( i );
				if ( pWearable && pWearable->GetAttributeContainer() )
				{
					CEconItemView *pItem = pWearable->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->IsValid() )
					{
						if ( ( pItem->GetItemDefIndex() == 743 ) ||	// Autogrant PyroVision Goggles
							( pItem->GetItemDefIndex() == 744 ) )		// PyroVision Goggles
						{
							IncrementCount();
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_DominateForGoggles, ACHIEVEMENT_TF_DOMINATE_FOR_GOGGLES, "TF_DOMINATE_FOR_GOGGLES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_ParachuteKillGrind : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			if ( pTFVictim->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_ParachuteKillGrind, ACHIEVEMENT_TF_PARACHUTE_KILL_GRIND, "TF_PARACHUTE_KILL_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MeleeKillClassicRifleSniper : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MeleeKillClassicRifleSniper, ACHIEVEMENT_TF_MELEE_KILL_CLASSIC_RIFLE_SNIPER, "TF_MELEE_KILL_CLASSIC_RIFLE_SNIPER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_KillChargingDemo : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			if ( pTFVictim->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_KillChargingDemo, ACHIEVEMENT_TF_KILL_CHARGING_DEMO, "TF_KILL_CHARGING_DEMO", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFTaunt_CongaKill : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "conga_kill" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "conga_kill" ) )
		{
			if ( event->GetInt( "index" ) == GetLocalPlayerIndex() )
			{
				int iNewIndex = m_Times.AddToTail();
				m_Times[iNewIndex] = gpGlobals->curtime;

				// we only care about the last three times we killed someone
				if ( m_Times.Count() > 3 )
				{
					m_Times.Remove( 0 );
				}

				if ( m_Times.Count() == 3 )
				{
					if ( m_Times.Tail() - m_Times.Head() <= 5.0 )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	CUtlVector< float >	m_Times;
};
DECLARE_ACHIEVEMENT( CAchievementTFTaunt_CongaKill, ACHIEVEMENT_TF_TAUNT_CONGA_KILL, "TF_TAUNT_CONGA_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFTaunt_CongaLine : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFTaunt_CongaLine, ACHIEVEMENT_TF_TAUNT_CONGA_LINE, "TF_TAUNT_CONGA_LINE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFTaunt_RPSRock : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iCount = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "rps_taunt_event" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "rps_taunt_event" ) )
		{
			if ( event->GetInt( "winner" ) == GetLocalPlayerIndex() )
			{
				m_iCount = 0;
			}
			else if ( event->GetInt( "loser" ) == GetLocalPlayerIndex() )
			{
				if ( event->GetInt( "loser_rps" ) == 0 ) // 0:rock 1:paper 2:scissors
				{
					m_iCount++;
					if ( m_iCount >= 3 )
					{
						IncrementCount();
					}
				}
				else
				{
					m_iCount = 0;
				}
			}
		}
	}

private:
	int m_iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTFTaunt_RPSRock, ACHIEVEMENT_TF_TAUNT_RPS_ROCK, "TF_TAUNT_RPS_ROCK", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFTaunt_RPSScissors : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iCount = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "rps_taunt_event" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "rps_taunt_event" ) )
		{
			if ( event->GetInt( "winner" ) == GetLocalPlayerIndex() )
			{
				if ( event->GetInt( "winner_rps" ) == 2 ) // 0:rock 1:paper 2:scissors
				{
					m_iCount++;
					if ( m_iCount >= 3 )
					{
						IncrementCount();
					}
				}
				else
				{
					m_iCount = 0;
				}
			}
			else if ( event->GetInt( "loser" ) == GetLocalPlayerIndex() )
			{
				m_iCount = 0;
			}
		}
	}

private:
	int m_iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTFTaunt_RPSScissors, ACHIEVEMENT_TF_TAUNT_RPS_SCISSORS, "TF_TAUNT_RPS_SCISSORS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFTaunt_DosidoMeleeKill : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFTaunt_DosidoMeleeKill, ACHIEVEMENT_TF_TAUNT_DOSIDO_MELLE_KILL, "TF_TAUNT_DOSIDO_MELLE_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFTaunt_TauntWhileCapping : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFTaunt_TauntWhileCapping, ACHIEVEMENT_TF_TAUNT_WHILE_CAPPING, "TF_TAUNT_WHILE_CAPPING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_PassTimeHat : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_PassTimeHat, ACHIEVEMENT_TF_PASS_TIME_HAT, "TF_PASS_TIME_HAT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_PassTimeGrind : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( TFGameRules() && TFGameRules()->IsPasstimeMode() )
		{
			if ( FStrEq( event->GetName(), "teamplay_round_win" ) )
			{
				// Were we on the winning team?
				int iTeam = event->GetInt( "team" );
				if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_PassTimeGrind, ACHIEVEMENT_TF_PASS_TIME_GRIND, "TF_PASS_TIME_GRIND", 5 );

#endif // CLIENT_DLL
