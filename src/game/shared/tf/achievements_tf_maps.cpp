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
#include "achievements_tf.h"
#include "c_team_objectiveresource.h"


//======================================================================================================================================
// FOUNDRY ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_KillCappingEnemy : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_KillCappingEnemy, ACHIEVEMENT_TF_MAPS_FOUNDRY_KILL_CAPPING_ENEMY, "TF_MAPS_FOUNDRY_KILL_CAPPING_ENEMY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_PlayGameFriends : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// are there at least 5 friends in the game? (at least 6 players total)
			if ( CalcPlayersOnFriendsList( 5 ) )
			{
				AwardAchievement();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_PlayGameFriends, ACHIEVEMENT_TF_MAPS_FOUNDRY_PLAY_GAME_FRIENDS, "TF_MAPS_FOUNDRY_PLAY_GAME_FRIENDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_WinMinTime : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				float flRoundTime = event->GetFloat( "round_time", 0 );
				if ( flRoundTime > 0 && flRoundTime < 2 * 60 )
				{
					AwardAchievement();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_WinMinTime, ACHIEVEMENT_TF_MAPS_FOUNDRY_WIN_MINTIME, "TF_MAPS_FOUNDRY_WIN_MINTIME", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_WinRounds : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 137 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_foundry" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_WinRounds, ACHIEVEMENT_TF_MAPS_FOUNDRY_WIN_ROUNDS, "TF_MAPS_FOUNDRY_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_FastFinalCap : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );

		ResetCounts();
	}

	void ResetCounts()
	{
		m_bRecentCapper = false;
		iCapCount = 0;
		iCapTimes[0] = 0.0f;
		iCapTimes[1] = 0.0f;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			m_bRecentCapper = false;
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == pLocalPlayer->GetTeamNumber() ) )
			{
				iCapTimes[iCapCount%2] = gpGlobals->curtime;
				iCapCount++;

				const char *cappers = event->GetString( "cappers" );
				for ( int i = 0 ; i < Q_strlen( cappers ) ; i++ )
				{
					int iPlayerIndex = (int) cappers[i];
					CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
					if ( pPlayer == pLocalPlayer )
					{
						m_bRecentCapper = true;
					}
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			// If we're the winners and we were involved in capping the last point, we get this achievement.
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == pLocalPlayer->GetTeamNumber() ) && m_bRecentCapper )
			{
				if ( fabs( iCapTimes[1] - iCapTimes[0] ) <= 5.0f )
				{
					AwardAchievement();
				}
			}

			ResetCounts();
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			ResetCounts();
		}
	}

private:
	bool m_bRecentCapper;
	int iCapCount;
	float iCapTimes[2];
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_FastFinalCap, ACHIEVEMENT_TF_MAPS_FOUNDRY_FAST_FINAL_CAP, "TF_MAPS_FOUNDRY_FAST_FINAL_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_TeleportAndCap : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_captured" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( !pLocalPlayer->m_Shared.InCond( TF_COND_TELEPORTED ) )
			return;

		if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == pLocalPlayer->GetTeamNumber() ) )
			{
				const char *cappers = event->GetString( "cappers" );
				for ( int i = 0 ; i < Q_strlen( cappers ) ; i++ )
				{
					int iPlayerIndex = (int) cappers[i];
					CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
					if ( pPlayer == pLocalPlayer )
					{
						AwardAchievement();
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_TeleportAndCap, ACHIEVEMENT_TF_MAPS_FOUNDRY_TELEPORT_AND_CAP, "TF_MAPS_FOUNDRY_TELEPORT_AND_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_PushIntoCauldron : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_PushIntoCauldron, ACHIEVEMENT_TF_MAPS_FOUNDRY_PUSH_INTO_CAULDRON, "TF_MAPS_FOUNDRY_PUSH_INTO_CAULDRON", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_PushBackAndWin : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );

		m_bFinalPointContested = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "teamplay_round_start" );
		ListenForGameEvent( "localplayer_changeteam" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				if ( m_bFinalPointContested )
				{
					AwardAchievement();
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_point_startcapture" ) )
		{
			if ( ObjectiveResource() && ( ObjectiveResource()->GetBaseControlPointForTeam( GetLocalPlayerTeam() ) == event->GetInt( "cp" ) ) )
			{
				m_bFinalPointContested = true;
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) || FStrEq( pszEventName, "localplayer_changeteam" ) )
		{
			m_bFinalPointContested = false;
		}
	}

private:
	bool m_bFinalPointContested;
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_PushBackAndWin, ACHIEVEMENT_TF_MAPS_FOUNDRY_PUSH_BACK_AND_WIN, "TF_MAPS_FOUNDRY_PUSH_BACK_AND_WIN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_PlayEachClass : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );

		m_iClassesPlayed = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_start" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pLocalPlayer && ( pLocalPlayer == pAttacker ) && ( pVictim != pAttacker ) && ( pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED ) )
		{
			m_iClassesPlayed |= ( 1 << pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() );

			if ( m_iClassesPlayed == 1022 )
			{
				AwardAchievement();
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_start" ) )
		{
			m_iClassesPlayed = 0;
		}
	}

private:
	int m_iClassesPlayed;
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_PlayEachClass, ACHIEVEMENT_TF_MAPS_FOUNDRY_PLAY_EACH_CLASS, "TF_MAPS_FOUNDRY_PLAY_EACH_CLASS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_KillEnemyOnRoof : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );

		iKillCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_start" );
		ListenForGameEvent( "player_killed_achievement_zone" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pLocalPlayer && ( pLocalPlayer == pVictim ) )
		{
			iKillCount = 0;
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			iKillCount = 0;
		}
		else if ( FStrEq( pszEventName, "player_killed_achievement_zone" ) )
		{
			CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
			CTFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "attacker" ) ) );
			CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "victim" ) ) );

			if ( pLocalPlayer && ( pLocalPlayer == pAttacker ) && pVictim && ( pVictim->GetTeamNumber() != pAttacker->GetTeamNumber() ) )
			{
				iKillCount++;
				if ( iKillCount >= 2 )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	int iKillCount;
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_KillEnemyOnRoof, ACHIEVEMENT_TF_MAPS_FOUNDRY_KILL_ENEMY_ON_ROOF, "TF_MAPS_FOUNDRY_KILL_ENEMY_ON_ROOF", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_BackAndForthBattle : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_foundry" );

		m_iCapCount = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			return;

		if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			// we don't care which team is capping or which point is capped for this achievement
			m_iCapCount++;

			if ( m_iCapCount >= 15 )
			{
				AwardAchievement();
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			m_iCapCount = 0;
		}
	}

private:
	int m_iCapCount;
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_BackAndForthBattle, ACHIEVEMENT_TF_MAPS_FOUNDRY_BACK_AND_FORTH_BATTLE, "TF_MAPS_FOUNDRY_BACK_AND_FORTH_BATTLE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFFoundry_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFFoundry_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 7, ACHIEVEMENT_TF_MAPS_FOUNDRY_START_RANGE, ACHIEVEMENT_TF_MAPS_FOUNDRY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFFoundry_AchieveProgress1, ACHIEVEMENT_TF_MAPS_FOUNDRY_ACHIEVE_PROGRESS1, "TF_MAPS_FOUNDRY_ACHIEVE_PROGRESS1", 5 );


//======================================================================================================================================
// DOOMSDAY ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_SoloCapture : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "sd_doomsday" );

		m_bCarriedFromHome = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_flag_event" ) )
		{
			int iPlayer = event->GetInt( "player" );
			int iType = event->GetInt( "eventtype" );
			bool bHome = ( event->GetInt( "home", 0 ) == 1 );

			switch( iType )
			{
			case TF_FLAGEVENT_PICKUP:
				if ( ( iPlayer == GetLocalPlayerIndex() ) && bHome )
				{
					m_bCarriedFromHome = true;
				}
				else
				{
					m_bCarriedFromHome = false;
				}
				break;
			case TF_FLAGEVENT_CAPTURE:
				if ( ( iPlayer == GetLocalPlayerIndex() ) && m_bCarriedFromHome )
				{
					IncrementCount();
				}
				break;
			default:
				m_bCarriedFromHome = false;
				break;
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			m_bCarriedFromHome = false;
		}
	}

private: 
	bool	m_bCarriedFromHome;
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_SoloCapture, ACHIEVEMENT_TF_MAPS_DOOMSDAY_SOLO_CAPTURE, "TF_MAPS_DOOMSDAY_SOLO_CAPTURE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_PlayGameFriends : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "sd_doomsday" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// are there at least 5 friends in the game? (at least 6 players total)
			if ( CalcPlayersOnFriendsList( 5 ) )
			{
				AwardAchievement();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_PlayGameFriends, ACHIEVEMENT_TF_MAPS_DOOMSDAY_PLAY_GAME_FRIENDS, "TF_MAPS_DOOMSDAY_PLAY_GAME_FRIENDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_WinRounds : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 138 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "sd_doomsday" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_WinRounds, ACHIEVEMENT_TF_MAPS_DOOMSDAY_WIN_ROUNDS, "TF_MAPS_DOOMSDAY_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_PlayEachClass : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS );
		SetGoal( ( TF_LAST_NORMAL_CLASS - 1 ) - TF_FIRST_NORMAL_CLASS + 1 ); //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
		SetMapNameFilter( "sd_doomsday" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_flag_event" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_flag_event" ) )
		{
			int iPlayer = event->GetInt( "player" );
			int iType = event->GetInt( "eventtype" );
	
			switch( iType )
			{
			case TF_FLAGEVENT_CAPTURE:
				if ( iPlayer == GetLocalPlayerIndex() )
				{
					C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
					if ( pTFPlayer )
					{
						int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
						if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= ( TF_LAST_NORMAL_CLASS - 1 ) ) //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
						{
							// yes, the achievement is satisfied for this class, set the corresponding bit	    
							int iBitNumber = ( iClass - TF_FIRST_NORMAL_CLASS );
							EnsureComponentBitSetAndEvaluate( iBitNumber );
						}							
					}
				}
				break;
			default:
				break;
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_PlayEachClass, ACHIEVEMENT_TF_MAPS_DOOMSDAY_PLAY_EACH_CLASS, "TF_MAPS_DOOMSDAY_PLAY_EACH_CLASS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_KillEnemiesOnElevator : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "sd_doomsday" );

		m_flTimeWindow = 10.0f;
		m_nKillsToAchieve = 3;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_killed_achievement_zone" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_killed_achievement_zone" ) )
		{
			int iZoneID = event->GetInt( "zone_id" );
			if ( iZoneID == 1 ) // capture zone
			{
				CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
				CTFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "attacker" ) ) );
				CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "victim" ) ) );

				if ( pLocalPlayer && ( pLocalPlayer == pAttacker ) && pVictim && ( pVictim->GetTeamNumber() != pAttacker->GetTeamNumber() ) )
				{
					int index = m_History.AddToHead();
					m_History[index] = gpGlobals->curtime;
					Evaluate();
				}
			}
		}
	}

	void Evaluate( void )
	{
		// remove any times that are older than the window
		float flTimeDiscard = gpGlobals->curtime - m_flTimeWindow;
		for ( int i = 0 ; i < m_History.Count() ; i++ )
		{
			if ( m_History[i] < flTimeDiscard )
			{
				m_History.RemoveMultiple( i, m_History.Count() - i );
				break;
			}
		}

		// have we killed enough players in the time window?
		if ( m_History.Count() >= m_nKillsToAchieve )
		{
			IncrementCount();
		}
	}
	 
private:
	CUtlVector< float >	m_History;
	float m_flTimeWindow;
	int m_nKillsToAchieve;
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_KillEnemiesOnElevator, ACHIEVEMENT_TF_MAPS_DOOMSDAY_KILL_ENEMIES_ON_ELEVATOR, "TF_MAPS_DOOMSDAY_KILL_ENEMIES_ON_ELEVATOR", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_PushBackAndWin : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "sd_doomsday" );

		m_bRocketOpenedByEnemyTeam = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "doomsday_rocket_open" );
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				if ( m_bRocketOpenedByEnemyTeam )
				{
					AwardAchievement();
				}
			}
		}
		else if ( FStrEq( pszEventName, "doomsday_rocket_open" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam != GetLocalPlayerTeam() ) )
			{
				m_bRocketOpenedByEnemyTeam = true;
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			m_bRocketOpenedByEnemyTeam = false;
		}
	}

private:
	bool m_bRocketOpenedByEnemyTeam;
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_PushBackAndWin, ACHIEVEMENT_TF_MAPS_DOOMSDAY_PUSH_BACK_AND_WIN, "TF_MAPS_DOOMSDAY_PUSH_BACK_AND_WIN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_KillCarriers : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		SetMapNameFilter( "sd_doomsday" );
		m_iKillCount = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
			return;

		if ( FStrEq( event->GetName(), "teamplay_round_start" ) )
		{
			m_iKillCount = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && pVictim != C_BasePlayer::GetLocalPlayer() )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && pTFVictim->HasTheFlag() )
			{
				m_iKillCount++;
				if ( m_iKillCount >= 6 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	int m_iKillCount;
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_KillCarriers, ACHIEVEMENT_TF_MAPS_DOOMSDAY_KILL_CARRIERS, "TF_MAPS_DOOMSDAY_KILL_CARRIERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_RideTheElevator : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_RideTheElevator, ACHIEVEMENT_TF_MAPS_DOOMSDAY_RIDE_THE_ELEVATOR, "TF_MAPS_DOOMSDAY_RIDE_THE_ELEVATOR", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_DenyNeutralPickup : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_DenyNeutralPickup, ACHIEVEMENT_TF_MAPS_DOOMSDAY_DENY_NEUTRAL_PICKUP, "TF_MAPS_DOOMSDAY_DENY_NEUTRAL_PICKUP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_PushIntoExhaust : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_PushIntoExhaust, ACHIEVEMENT_TF_MAPS_DOOMSDAY_PUSH_INTO_EXHAUST, "TF_MAPS_DOOMSDAY_PUSH_INTO_EXHAUST", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_DefendCarrier : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_DefendCarrier, ACHIEVEMENT_TF_MAPS_DOOMSDAY_DEFEND_CARRIER, "TF_MAPS_DOOMSDAY_DEFEND_CARRIER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDoomsday_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFDoomsday_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 7, ACHIEVEMENT_TF_MAPS_DOOMSDAY_START_RANGE, ACHIEVEMENT_TF_MAPS_DOOMSDAY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDoomsday_AchieveProgress1, ACHIEVEMENT_TF_MAPS_DOOMSDAY_ACHIEVE_PROGRESS1, "TF_MAPS_DOOMSDAY_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFStandin_WinRounds : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 139 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_standin_final" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFStandin_WinRounds, ACHIEVEMENT_TF_MAPS_STANDIN_WIN_ROUNDS, "TF_MAPS_STANDIN_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFProcess_WinRounds : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 140 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_process_final" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFProcess_WinRounds, ACHIEVEMENT_TF_MAPS_PROCESS_WIN_ROUNDS, "TF_MAPS_PROCESS_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSnakewater_WinRounds : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 141 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_snakewater_final1" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSnakewater_WinRounds, ACHIEVEMENT_TF_MAPS_SNAKEWATER_WIN_ROUNDS, "TF_MAPS_SNAKEWATER_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSnakewater_PushBackAndWin : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_snakewater_final1" );

		m_bFinalPointContested = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				if ( m_bFinalPointContested )
				{
					AwardAchievement();
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_point_startcapture" ) )
		{
			if ( ObjectiveResource() && ( ObjectiveResource()->GetBaseControlPointForTeam( GetLocalPlayerTeam() ) == event->GetInt( "cp" ) ) )
			{
				m_bFinalPointContested = true;
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			m_bFinalPointContested = false;
		}
	}

private:
	bool m_bFinalPointContested;
};
DECLARE_ACHIEVEMENT( CAchievementTFSnakewater_PushBackAndWin, ACHIEVEMENT_TF_MAPS_SNAKEWATER_PUSH_BACK_AND_WIN, "TF_MAPS_SNAKEWATER_PUSH_BACK_AND_WIN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSnakewater_TeamKill : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		SetMapNameFilter( "cp_snakewater_final1" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( TFGameRules() && ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) )
		{
			if ( pAttacker && ( pAttacker->GetTeamNumber() == GetLocalPlayerTeam() ) )
			{
				C_TFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
				
				if ( pTFAttacker && pTFAttacker->GetTeam() && pTFVictim && pTFVictim->GetTeam() )
				{
					// must have 12 or more players on the server
					if ( pTFAttacker->GetTeam()->GetNumPlayers() + pTFVictim->GetTeam()->GetNumPlayers() >= 12 )
					{
						bool bSomeAlive = false;
						int nTeamCount = pTFVictim->GetTeam()->GetNumPlayers();
						for ( int i = 0; i < nTeamCount; i++ )
						{
							C_BasePlayer *pTemp = pTFVictim->GetTeam()->GetPlayer( i );
							if ( pTemp && ( pTemp != pTFVictim ) && pTemp->IsAlive() )
							{
								// Found one
								bSomeAlive = true;
								break;
							}
						}

						if ( !bSomeAlive )
						{
							AwardAchievement();
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSnakewater_TeamKill, ACHIEVEMENT_TF_MAPS_SNAKEWATER_TEAM_KILL, "TF_MAPS_SNAKEWATER_TEAM_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSnakewater_DoubleAirDeaths : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
		SetMapNameFilter( "cp_snakewater_final1" );
		ResetTracking();
	}

	void ResetTracking()
	{
		m_PotentialPartners.RemoveAll();
		m_nKilledLocalPlayer = -1;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "player_spawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			ResetTracking();
		}
		else if ( FStrEq( pszEvent, "player_spawn" ) )
		{
			int nUserId = event->GetInt( "userid" );
			if ( nUserId > 0 )
			{
				int iIndex = m_PotentialPartners.Find( nUserId );
				if ( iIndex != m_PotentialPartners.InvalidIndex() )
				{
					m_PotentialPartners.Remove( iIndex );
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		C_TFPlayer *pTFLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		// was the victim rocket jumping?
		if ( event->GetBool( "rocket_jump" ) )
		{
			if ( pTFAttacker && pTFVictim && ( pTFAttacker != pTFVictim ) && ( ( pTFAttacker == pTFLocalPlayer ) || ( pTFVictim == pTFLocalPlayer ) ) )
			{
				int iWeaponID = event->GetInt( "weaponid" );
				if ( ( iWeaponID == TF_WEAPON_ROCKETLAUNCHER ) || ( iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT ) )
				{		
					if ( pTFAttacker == pTFLocalPlayer )
					{
						if ( m_PotentialPartners.Find( pTFVictim->GetUserID() ) == m_PotentialPartners.InvalidIndex() )
						{
							m_PotentialPartners.AddToTail( pTFVictim->GetUserID() );
						}
					}
					else if ( pTFVictim == pTFLocalPlayer )
					{
						m_nKilledLocalPlayer = pTFAttacker->GetUserID();
					}

					// evaluate the achievement
					if ( ( m_nKilledLocalPlayer > -1 ) && ( m_PotentialPartners.Find( m_nKilledLocalPlayer ) != m_PotentialPartners.InvalidIndex() ) )
					{
						AwardAchievement();
					}
				}
			}
		}
	}

	CUtlVector< int > m_PotentialPartners; // userIDs of the players the localPlayer killed
	int m_nKilledLocalPlayer; // userID of the player who killed the localPlayer
};
DECLARE_ACHIEVEMENT( CAchievementTFSnakewater_DoubleAirDeaths, ACHIEVEMENT_TF_MAPS_SNAKEWATER_DOUBLE_AIR_DEATHS, "TF_MAPS_SNAKEWATER_DOUBLE_AIR_DEATHS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSnakewater_KillEnemiesInMiddle : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 15 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_snakewater_final1" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_killed_achievement_zone" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_killed_achievement_zone" ) )
		{
			CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
			CTFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "attacker" ) ) );
			CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "victim" ) ) );

			if ( pLocalPlayer && ( pLocalPlayer == pAttacker ) && pVictim && ( pVictim->GetTeamNumber() != pAttacker->GetTeamNumber() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSnakewater_KillEnemiesInMiddle, ACHIEVEMENT_TF_MAPS_SNAKEWATER_KILL_ENEMIES_IN_MIDDLE, "TF_MAPS_SNAKEWATER_KILL_ENEMIES_IN_MIDDLE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPowerhouse_WinRounds : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 142 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_powerhouse" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPowerhouse_WinRounds, ACHIEVEMENT_TF_MAPS_POWERHOUSE_WIN_ROUNDS, "TF_MAPS_POWERHOUSE_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPowerhouse_PushBackAndWin : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_powerhouse" );

		m_bFinalPointContested = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "teamplay_round_start" );
		ListenForGameEvent( "localplayer_changeteam" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				if ( m_bFinalPointContested )
				{
					AwardAchievement();
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_point_startcapture" ) )
		{
			if ( ObjectiveResource() && ( ObjectiveResource()->GetBaseControlPointForTeam( GetLocalPlayerTeam() ) == event->GetInt( "cp" ) ) )
			{
				m_bFinalPointContested = true;
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) || FStrEq( pszEventName, "localplayer_changeteam" ) )
		{
			m_bFinalPointContested = false;
		}
	}

private:
	bool m_bFinalPointContested;
};
DECLARE_ACHIEVEMENT( CAchievementTFPowerhouse_PushBackAndWin, ACHIEVEMENT_TF_MAPS_POWERHOUSE_PUSH_BACK_AND_WIN, "TF_MAPS_POWERHOUSE_PUSH_BACK_AND_WIN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPowerhouse_FastFinalCap : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "cp_powerhouse" );

		ResetCounts();
	}

	void ResetCounts()
	{
		m_bRecentCapper = false;
		iCapCount = 0;
		iCapTimes[0] = 0.0f;
		iCapTimes[1] = 0.0f;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_round_start" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			m_bRecentCapper = false;
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == pLocalPlayer->GetTeamNumber() ) )
			{
				iCapTimes[iCapCount % 2] = gpGlobals->curtime;
				iCapCount++;

				const char *cappers = event->GetString( "cappers" );
				for ( int i = 0; i < Q_strlen( cappers ); i++ )
				{
					int iPlayerIndex = (int)cappers[i];
					CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
					if ( pPlayer == pLocalPlayer )
					{
						m_bRecentCapper = true;
					}
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			// If we're the winners and we were involved in capping the last point, we get this achievement.
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == pLocalPlayer->GetTeamNumber() ) && m_bRecentCapper )
			{
				if ( fabs( iCapTimes[1] - iCapTimes[0] ) <= 15.0f )
				{
					AwardAchievement();
				}
			}

			ResetCounts();
		}
		else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
		{
			ResetCounts();
		}
	}

private:
	bool m_bRecentCapper;
	int iCapCount;
	float iCapTimes[2];
};
DECLARE_ACHIEVEMENT( CAchievementTFPowerhouse_FastFinalCap, ACHIEVEMENT_TF_MAPS_POWERHOUSE_FAST_FINAL_CAP, "TF_MAPS_POWERHOUSE_FAST_FINAL_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPowerhouse_KillCappingPlayer : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 15 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_powerhouse" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "killed_capping_player" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "killed_capping_player" ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				int iKiller = event->GetInt( "killer", 0 );
				if ( iKiller == pLocalPlayer->entindex() )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPowerhouse_KillCappingPlayer, ACHIEVEMENT_TF_MAPS_POWERHOUSE_KILL_CAPPING_PLAYER, "TF_MAPS_POWERHOUSE_KILL_CAPPING_PLAYER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPowerhouse_KillEnemyInWater : public CBaseTFAchievementSimple
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "cp_powerhouse" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pAttacker == pLocalPlayer && pVictim != pLocalPlayer )
		{
			if ( pVictim->GetWaterLevel() != WL_NotInWater )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPowerhouse_KillEnemyInWater, ACHIEVEMENT_TF_MAPS_POWERHOUSE_KILL_ENEMY_IN_WATER, "TF_MAPS_POWERHOUSE_KILL_ENEMY_IN_WATER", 5 );

#endif // CLIENT_DLL
