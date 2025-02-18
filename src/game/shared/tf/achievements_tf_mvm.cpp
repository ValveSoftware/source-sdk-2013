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
#include "c_tf_objective_resource.h"
#include "tf_gamerules.h"
#include "achievements_tf.h"
#include "tf_item_powerup_bottle.h"

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_CompletePopFile : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_bPlayedEntireMission = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_mission_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			if ( event->GetInt( "wave_index" ) == 0 )
			{
				m_bPlayedEntireMission = true;
			}
		}
		else if ( FStrEq( pszEventName, "mvm_mission_complete" ) )
		{
			if ( m_bPlayedEntireMission )
			{
				AwardAchievement();
			}
		}
	}

private:
	bool m_bPlayedEntireMission;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_CompletePopFile, ACHIEVEMENT_TF_MVM_COMPLETE_POP_FILE, "TF_MVM_COMPLETE_POP_FILE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_EarnMoneyBonus : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_bPlayedEntireWave = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "mvm_creditbonus_wave" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			m_bPlayedEntireWave = true;
		}
		else if ( FStrEq( pszEventName, "mvm_creditbonus_wave" ) )
		{
			if ( m_bPlayedEntireWave )
			{
				AwardAchievement();
			}
		}
	}

private:
	bool m_bPlayedEntireWave;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_EarnMoneyBonus, ACHIEVEMENT_TF_MVM_EARN_MONEY_BONUS, "TF_MVM_EARN_MONEY_BONUS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_AdvancedEarnAllBonuses : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_bPlayedEntireMission = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_creditbonus_all_advanced" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			if ( event->GetInt( "wave_index" ) == 0 )
			{
				m_bPlayedEntireMission = true;
			}
		}
		else if ( FStrEq( pszEventName, "mvm_creditbonus_all_advanced" ) )
		{
			if ( m_bPlayedEntireMission )
			{
				AwardAchievement();
			}
		}
	}

private:
	bool m_bPlayedEntireMission;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_AdvancedEarnAllBonuses, ACHIEVEMENT_TF_MVM_ADVANCED_EARN_ALL_BONUSES, "TF_MVM_ADVANCED_EARN_ALL_BONUSES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_PickupMoneyAboutToExpire : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_PickupMoneyAboutToExpire, ACHIEVEMENT_TF_MVM_PICKUP_MONEY_ABOUT_TO_EXPIRE, "TF_MVM_PICKUP_MONEY_ABOUT_TO_EXPIRE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_CollectMoneyGrind : public CBaseTFAchievementSimple
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000000 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_pickup_currency" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_pickup_currency" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				IncrementCount( event->GetInt( "currency" ) );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_CollectMoneyGrind, ACHIEVEMENT_TF_MVM_COLLECT_MONEY_GRIND, "TF_MVM_COLLECT_MONEY_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_PlayGameFriends : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_mission_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_mission_complete" ) )
		{
			if ( CalcPlayersOnFriendsList( 5 ) )
			{
				AwardAchievement();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_PlayGameFriends, ACHIEVEMENT_TF_MVM_PLAY_GAME_FRIENDS, "TF_MVM_PLAY_GAME_FRIENDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_PlayEachClass : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS );
		SetGoal( ( TF_LAST_NORMAL_CLASS - 1 ) - TF_FIRST_NORMAL_CLASS + 1 ); //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class

		m_bChangedClass = true;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "localplayer_changeclass" );
		ListenForGameEvent( "localplayer_changeteam" );
		ListenForGameEvent( "mvm_mission_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			if ( event->GetInt( "wave_index" ) == 0 )
			{
				// beginning the first wave...lock down class changes
				m_bChangedClass = false;
			}
		}
		else if ( FStrEq( pszEventName, "localplayer_changeclass" ) ||	   // can't change class or team after the first round starts
				  FStrEq( pszEventName, "localplayer_changeteam" ) )
		{
			m_bChangedClass = true;
		}
		else if ( FStrEq( pszEventName, "mvm_mission_complete" ) )
		{
			if ( !m_bChangedClass )
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

private:
	bool m_bChangedClass;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_PlayEachClass, ACHIEVEMENT_TF_MVM_PLAY_EACH_CLASS, "TF_MVM_PLAY_EACH_CLASS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_DestroyTwoTanks : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_flLastTankDestroyedTime = 0.0f;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_tank_destroyed_by_players" );
		ListenForGameEvent( "mvm_begin_wave" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_tank_destroyed_by_players" ) )
		{
			if ( m_flLastTankDestroyedTime > 0.0f )
			{
				if ( fabs( gpGlobals->curtime - m_flLastTankDestroyedTime ) <= 5.0f )
				{
					AwardAchievement();
				}
			}

			m_flLastTankDestroyedTime = gpGlobals->curtime;
		}

		if ( FStrEq( event->GetName(), "mvm_begin_wave" ) )
		{
			m_flLastTankDestroyedTime = 0.f;
		}
	}

private:
	float m_flLastTankDestroyedTime;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_DestroyTwoTanks, ACHIEVEMENT_TF_MVM_DESTROY_TWO_TANKS, "TF_MVM_DESTROY_TWO_TANKS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_DestroyTankWhileDeploying : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_DestroyTankWhileDeploying, ACHIEVEMENT_TF_MVM_DESTROY_TANK_WHILE_DEPLOYING, "TF_MVM_DESTROY_TANK_WHILE_DEPLOYING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_DestroyTankQuickly : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_DestroyTankQuickly, ACHIEVEMENT_TF_MVM_DESTROY_TANK_QUICKLY, "TF_MVM_DESTROY_TANK_QUICKLY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_DefendCap : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		iCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "mvm_kill_robot_delivering_bomb" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			iCount = 0;
		}
		else if ( FStrEq( pszEventName, "mvm_kill_robot_delivering_bomb" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				iCount++;
				if ( iCount >= 10 )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	int iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_DefendCap, ACHIEVEMENT_TF_MVM_DEFEND_CAP, "TF_MVM_DEFEND_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_KillBombCarriers : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_nConsecutiveKillCount = 0;
		ACHIEVEMENT_COUNT = 15;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_bomb_carrier_killed" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "mvm_bomb_carrier_killed" ) == 0 )
		{
			if ( GetLocalPlayerTeam() != TF_TEAM_PVE_DEFENDERS )
				return;

			int nLevel = event->GetInt( "level" );
			if ( !nLevel )
			{
				m_nConsecutiveKillCount++;
			}
			else
			{
				m_nConsecutiveKillCount = 0;
			}
		}
		else if ( Q_strcmp( event->GetName(), "teamplay_round_active" ) == 0 )
		{
			m_nConsecutiveKillCount = 0;
		}

		if ( m_nConsecutiveKillCount >= ACHIEVEMENT_COUNT )
		{
			AwardAchievement();
		}
	}

private:
	int m_nConsecutiveKillCount;
	int ACHIEVEMENT_COUNT;

};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_KillBombCarriers, ACHIEVEMENT_TF_MVM_KILL_BOMB_CARRIERS, "TF_MVM_KILL_BOMB_CARRIERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_CompleteWaveWithoutDying : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );

		bSurvivedEntireWave = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "mvm_wave_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			bSurvivedEntireWave = true;
		}
		else if ( FStrEq( pszEventName, "mvm_wave_complete" ) )
		{
			if ( event->GetBool( "advanced" ) )
			{
				if ( bSurvivedEntireWave )
				{
					AwardAchievement();
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pVictim && ( pVictim == C_BasePlayer::GetLocalPlayer() ) )
		{
			bSurvivedEntireWave = false;
		}
	}

private:
	bool bSurvivedEntireWave;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_CompleteWaveWithoutDying, ACHIEVEMENT_TF_MVM_COMPLETE_WAVE_WITHOUT_DYING, "TF_MVM_COMPLETE_WAVE_WITHOUT_DYING", 5 );
 
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_CompleteTour : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS );

		static const char *szComponents[] =
		{
			"scripts/population/mvm_decoy.pop", "scripts/population/mvm_coaltown.pop", "scripts/population/mvm_mannworks.pop"
		};		
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetGoal( m_iNumComponents );

		m_bPlayedEntireMission = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_mission_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			if ( event->GetInt( "wave_index" ) == 0 )
			{
				m_bPlayedEntireMission = true;
			}
		}
		else if ( FStrEq( pszEventName, "mvm_mission_complete" ) )
		{
			if ( m_bPlayedEntireMission )
			{
				OnComponentEvent( event->GetString( "mission" ) );
			}
		}
	}

private:
	bool m_bPlayedEntireMission;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_CompleteTour, ACHIEVEMENT_TF_MVM_COMPLETE_TOUR, "TF_MVM_COMPLETE_TOUR", 5 );
 
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_UseTeleportBottle : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_flAchievementEndTime = 0.0f;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_used_powerup_bottle" );
		ListenForGameEvent( "teamplay_flag_event" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_used_powerup_bottle" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				if ( event->GetInt( "type" ) == POWERUP_BOTTLE_RECALL )
				{
					// defend the bomb within 5 seconds
					m_flAchievementEndTime = gpGlobals->curtime + 5.0f;
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_flag_event" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				if ( event->GetInt( "eventtype" ) == TF_FLAGEVENT_DEFEND )
				{
					if ( gpGlobals->curtime < m_flAchievementEndTime )
					{
						AwardAchievement();
					}
				}
			}
		}
	}

private:
	float m_flAchievementEndTime;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_UseTeleportBottle, ACHIEVEMENT_TF_MVM_USE_TELEPORT_BOTTLE, "TF_MVM_USE_TELEPORT_BOTTLE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_UseCritBottle : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		m_flAchievementEndTime = 0.0f;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_used_powerup_bottle" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_used_powerup_bottle" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				if ( event->GetInt( "type" ) == POWERUP_BOTTLE_CRITBOOST )
				{
					C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
					if ( pLocalTFPlayer )
					{
						m_flAchievementEndTime = gpGlobals->curtime + event->GetFloat( "time" );
					}
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( gpGlobals->curtime < m_flAchievementEndTime )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsMiniBoss() )
			{
				AwardAchievement();
			}
		}
	}

private:
	float m_flAchievementEndTime;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_UseCritBottle, ACHIEVEMENT_TF_MVM_USE_CRIT_BOTTLE, "TF_MVM_USE_CRIT_BOTTLE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_UseUberBottle : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		m_iKillCount = 0;
		m_flAchievementEndTime = 0.0f;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_used_powerup_bottle" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_used_powerup_bottle" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				if ( event->GetInt( "type" ) == POWERUP_BOTTLE_UBERCHARGE )
				{
					m_flAchievementEndTime = gpGlobals->curtime + event->GetFloat( "time" );
					m_iKillCount = 0;
				}
			}
 		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( gpGlobals->curtime < m_flAchievementEndTime )
		{
			m_iKillCount++;
			if ( m_iKillCount >= 15 )
			{
				AwardAchievement();
			}
		}
	}

private:
	float m_flAchievementEndTime;
	int m_iKillCount;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_UseUberBottle, ACHIEVEMENT_TF_MVM_USE_UBER_BOTTLE, "TF_MVM_USE_UBER_BOTTLE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_UseBuildBottle : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_flAchievementEndTime = 0.0f;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "object_destroyed" );
		ListenForGameEvent( "mvm_quick_sentry_upgrade" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "object_destroyed" ) )
		{
			if ( TFGameRules() && ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) )
			{
				int iObject = event->GetInt( "objecttype" );
				if ( iObject == OBJ_SENTRYGUN )
				{
					int iEngineerIdx = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
					if ( iEngineerIdx == GetLocalPlayerIndex() )
					{
						m_flAchievementEndTime = gpGlobals->curtime + 3.0f;
					}
				}
			}
		}
		else if ( FStrEq( pszEventName, "mvm_quick_sentry_upgrade" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				if ( gpGlobals->curtime < m_flAchievementEndTime )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	float m_flAchievementEndTime;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_UseBuildBottle, ACHIEVEMENT_TF_MVM_USE_BUILD_BOTTLE, "TF_MVM_USE_BUILD_BOTTLE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_UseAmmoBottle : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_UseAmmoBottle, ACHIEVEMENT_TF_MVM_USE_AMMO_BOTTLE, "TF_MVM_USE_AMMO_BOTTLE", 5 );
 
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_MaxPrimaryUpgrades : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_MaxPrimaryUpgrades, ACHIEVEMENT_TF_MVM_MAX_PRIMARY_UPGRADES, "TF_MVM_MAX_PRIMARY_UPGRADES", 5 );
 
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_MaxPlayerResistances : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_MaxPlayerResistances, ACHIEVEMENT_TF_MVM_MAX_PLAYER_RESISTANCES, "TF_MVM_MAX_PLAYER_RESISTANCES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_NoAlarmsInFinalWave : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_bAlarmTriggered = true;
	}
 
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_bomb_alarm_triggered" );
		ListenForGameEvent( "mvm_mission_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			if ( event->GetInt( "advanced" ) > 0 )
			{
				if ( event->GetInt( "wave_index" ) == ( event->GetInt( "max_waves" ) - 1 ) )
				{
					m_bAlarmTriggered = false;
				}
				else
				{
					m_bAlarmTriggered = true;
				}
			}
		}
		else if ( FStrEq( pszEventName, "mvm_bomb_alarm_triggered" ) )
		{
			m_bAlarmTriggered = true;
		}
		else if ( FStrEq( pszEventName, "mvm_mission_complete" ) )
		{
			if ( !m_bAlarmTriggered )
			{
				AwardAchievement();
			}
		}
	}

private:
	bool m_bAlarmTriggered;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_NoAlarmsInFinalWave, ACHIEVEMENT_TF_MVM_NO_ALARMS_IN_FINAL_WAVE, "TF_MVM_NO_ALARMS_IN_FINAL_WAVE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_KillMedicsCharged : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		iCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			iCount = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_MEDIC ) && ( pTFVictim->MedicGetChargeLevel() >= 1.0 ) )
		{
			iCount++;

			if ( iCount >= 5 )
			{
				AwardAchievement();
			}
		}
	}

private:
	int iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_KillMedicsCharged, ACHIEVEMENT_TF_MVM_KILL_MEDICS_CHARGED, "TF_MVM_KILL_MEDICS_CHARGED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_KillRobotGrind : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 100000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		IncrementCount();
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_KillRobotGrind, ACHIEVEMENT_TF_MVM_KILL_ROBOT_GRIND, "TF_MVM_KILL_ROBOT_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_KillRobotMegaGrind : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1000000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		IncrementCount();
	}	
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_KillRobotMegaGrind, ACHIEVEMENT_TF_MVM_KILL_ROBOT_MEGA_GRIND, "TF_MVM_KILL_ROBOT_MEGA_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_KillSentryBuster : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_KillSentryBuster, ACHIEVEMENT_TF_MVM_KILL_SENTRY_BUSTER, "TF_MVM_KILL_SENTRY_BUSTER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_SpySapRobots : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_SpySapRobots, ACHIEVEMENT_TF_MVM_SPY_SAP_ROBOTS, "TF_MVM_SPY_SAP_ROBOTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_SoldierBuffTeam : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_SoldierBuffTeam, ACHIEVEMENT_TF_MVM_SOLDIER_BUFF_TEAM, "TF_MVM_SOLDIER_BUFF_TEAM", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_HeavyRagePushDeployingRobot : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_bomb_deploy_reset_by_player" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_bomb_deploy_reset_by_player" ) )
		{
			if ( GetLocalPlayerTeam() != TF_TEAM_PVE_DEFENDERS )
				return;

			C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalTFPlayer )
				return;

			if ( !pLocalTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
				return;

			if ( !pLocalTFPlayer->m_Shared.IsRageDraining() )
				return;

			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				AwardAchievement();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_HeavyRagePushDeployingRobot, ACHIEVEMENT_TF_MVM_HEAVY_RAGE_PUSH_DEPLOYING_ROBOT, "TF_MVM_HEAVY_RAGE_PUSH_DEPLOYING_ROBOT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_MedicShareBottles : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		iCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "mvm_medic_powerup_shared" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			iCount = 0;
		}
		else if ( FStrEq( pszEventName, "mvm_medic_powerup_shared" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				iCount++;

				if ( iCount >= 5 )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	int iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_MedicShareBottles, ACHIEVEMENT_TF_MVM_MEDIC_SHARE_BOTTLES, "TF_MVM_MEDIC_SHARE_BOTTLES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_DemoGroupKill : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_DemoGroupKill, ACHIEVEMENT_TF_MVM_DEMO_GROUP_KILL, "TF_MVM_DEMO_GROUP_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_ScoutMarkForDeath : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		iCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "mvm_scout_marked_for_death" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			iCount = 0;
		}
		else if ( FStrEq( pszEventName, "mvm_scout_marked_for_death" ) )
		{
			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				iCount++;

				if ( iCount >= 15 )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	int iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_ScoutMarkForDeath, ACHIEVEMENT_TF_MVM_SCOUT_MARK_FOR_DEATH, "TF_MVM_SCOUT_MARK_FOR_DEATH", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_SniperKillGroup : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_SniperKillGroup, ACHIEVEMENT_TF_MVM_SNIPER_KILL_GROUP, "TF_MVM_SNIPER_KILL_GROUP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_PyroBombReset : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		RESET_COUNT = 3;
		m_iResetCountInWave = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_bomb_reset_by_player" );
		ListenForGameEvent( "mvm_wave_complete" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();
		if ( FStrEq( pszEventName, "mvm_bomb_reset_by_player" ) )
		{
			if ( GetLocalPlayerTeam() != TF_TEAM_PVE_DEFENDERS )
				return;

			C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalTFPlayer )
				return;

			if ( !pLocalTFPlayer->IsPlayerClass( TF_CLASS_PYRO ) )
				return;

			if ( event->GetInt( "player" ) == GetLocalPlayerIndex() )
			{
				m_iResetCountInWave++;
			}

			if ( m_iResetCountInWave >= RESET_COUNT )
			{
				AwardAchievement();
			}
		}
		else if ( FStrEq( pszEventName, "mvm_wave_complete" ) || 
				  FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			m_iResetCountInWave = 0;
		}
	}

private:

	int RESET_COUNT;
	int m_iResetCountInWave;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_PyroBombReset, ACHIEVEMENT_TF_MVM_PYRO_BOMB_RESET, "TF_MVM_PYRO_BOMB_RESET", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_EngineerEscapeSentryBuster : public CBaseTFAchievementSimple
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		ListenForGameEvent( "mvm_sentrybuster_detonate" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// We should only be here if the sentry buster thinks it was successful
		// which is to say started to detonate because they were within range
		if ( FStrEq( event->GetName(), "mvm_sentrybuster_detonate" ) )
		{
			if ( GetLocalPlayerTeam() != TF_TEAM_PVE_DEFENDERS )
				return;

			int iTargetIdx = event->GetInt( "player" );

			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer )
				return;

			if ( !pLocalPlayer->IsAlive() )
				return;

			if ( !pLocalPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
				return;

			// Where it exploded
			float x, y, z = 0;
			x = event->GetFloat( "det_x" );
			y = event->GetFloat( "det_y" );
			z = event->GetFloat( "det_z" );

			// If we're not the target, but within a reasonable range of the sentry buster, 
			// also give credit, otherwise we give the target credit for still being alive.
			if ( GetLocalPlayerIndex() != iTargetIdx )
			{
				Vector vDist = pLocalPlayer->GetAbsOrigin() - Vector( x, y, z );
				if ( vDist.LengthSqr() > 400 * 400 )
					return;
			}

			AwardAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_EngineerEscapeSentryBuster, ACHIEVEMENT_TF_MVM_ENGINEER_ESCAPE_SENTRY_BUSTER, "TF_MVM_ENGINEER_ESCAPE_SENTRY_BUSTER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Rottenburg_Tank : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Rottenburg_Tank, ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_TANK, "TF_MVM_MAPS_ROTTENBURG_TANK", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Rottenburg_Bomb : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "mvm_rottenburg" );
		m_bValidWave = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_wave_complete" );
		ListenForGameEvent( "flag_carried_in_detection_zone" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			m_bValidWave = true;
		}
		else if ( FStrEq( pszEventName, "flag_carried_in_detection_zone" ) )
		{
			m_bValidWave = false;
		}
		else if ( FStrEq( pszEventName, "mvm_wave_complete" ) )
		{
			if ( event->GetBool( "advanced" ) )
			{
				if ( m_bValidWave )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	bool m_bValidWave;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Rottenburg_Bomb, ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_BOMB, "TF_MVM_MAPS_ROTTENBURG_BOMB", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Rottenburg_PitGrind : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Rottenburg_PitGrind, ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_PIT_GRIND, "TF_MVM_MAPS_ROTTENBURG_PIT_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Manhattan_Pit : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "mvm_mannhattan" );
		m_iCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_mannhattan_pit" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			m_iCount = 0;
		}
		else if ( FStrEq( pszEventName, "mvm_mannhattan_pit" ) )
		{
			m_iCount++;
			if ( m_iCount >= 10 )
			{
				AwardAchievement();
			}
		}
	}

private:
	int m_iCount;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Manhattan_Pit, ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_PIT, "TF_MVM_MAPS_MANNHATTAN_PIT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Manhattan_Mystery : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Manhattan_Mystery, ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_MYSTERY, "TF_MVM_MAPS_MANNHATTAN_MYSTERY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Manhattan_NoGates : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "mvm_mannhattan" );
		m_iWaveBits = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_begin_wave" );
		ListenForGameEvent( "mvm_adv_wave_complete_no_gates" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "mvm_begin_wave" ) )
		{
			if ( event->GetInt( "wave_index" ) == 0 )
			{
				m_iWaveBits = 0;
			}
		}
		else if ( FStrEq( pszEventName, "mvm_adv_wave_complete_no_gates" ) )
		{
			m_iWaveBits |= ( 1 << event->GetInt( "index" ) );

			int iComponentBits = m_iWaveBits;
			int iNumBitsSet = 0;

			while ( iComponentBits > 0 )
			{
				if ( iComponentBits & 1 )
				{
					iNumBitsSet++;
				}
				iComponentBits >>= 1;
			}

			if ( TFObjectiveResource() )
			{
				if ( iNumBitsSet >= TFObjectiveResource()->GetMannVsMachineMaxWaveCount() )
				{
					AwardAchievement();
				}
			}
		}
	}

private:
	int m_iWaveBits;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Manhattan_NoGates, ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_NO_GATES, "TF_MVM_MAPS_MANNHATTAN_NO_GATES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Manhattan_KillStunRadiowave : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "mvm_mannhattan" );
		m_nRobotsKilled = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_adv_wave_killed_stun_radio" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_adv_wave_killed_stun_radio" ) )
		{
			m_nRobotsKilled++;
			if ( m_nRobotsKilled >= 50 )
			{
				AwardAchievement();
			}
		}
	}

private: 
	int m_nRobotsKilled;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Manhattan_KillStunRadiowave, ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_STUN_RADIOWAVE, "TF_MVM_MAPS_MANNHATTAN_STUN_RADIOWAVE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Maps_Manhattan_BombBotGrind : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 500 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Maps_Manhattan_BombBotGrind, ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_BOMB_BOT_GRIND, "TF_MVM_MAPS_MANNHATTAN_BOMB_BOT_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_SentryBusterFriendlyFire : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );

		m_flDetonateTime = 0.f;
		m_pSentryBuster = NULL;
		m_Victims.EnsureCapacity( MAX_PLAYERS );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_sentrybuster_killed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_sentrybuster_killed" ) )
		{
			m_pSentryBuster = UTIL_PlayerByIndex( event->GetInt( "sentry_buster" ) );
			if ( m_pSentryBuster )
			{
				m_flDetonateTime = gpGlobals->curtime;
				SetNextThink( 0.1 );
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker && pVictim )
		{
			CBasePlayer *pAttackerPlayer = UTIL_PlayerByIndex( pAttacker->entindex() );
			if ( m_pSentryBuster && m_pSentryBuster == pAttackerPlayer && gpGlobals->curtime <= m_flDetonateTime + 0.25f )
			{
				if ( pVictim->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
				{
					if ( m_Victims.Find( pVictim->entindex() ) == m_Victims.InvalidIndex() )
					{
						m_Victims.AddToTail( pVictim->entindex() );
					}
				}
			}
		}
	}

	virtual void Think( void )
	{
		if ( gpGlobals->curtime <= m_flDetonateTime + 0.25f )
		{
			int nVictims = m_Victims.Count();
			if ( nVictims >= 5 )
			{
				AwardAchievement();
			}

			SetNextThink( 0.1 );
			return;
		}

		m_pSentryBuster = NULL;
		m_Victims.RemoveAll();
	}

private:
	CUtlVector< int > m_Victims;
	CBasePlayer *m_pSentryBuster;
	float m_flDetonateTime;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_SentryBusterFriendlyFire, ACHIEVEMENT_TF_MVM_SENTRY_BUSTER_FRIENDLY_FIRE, "TF_MVM_SENTRY_BUSTER_FRIENDLY_FIRE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Sniper_CollectHeadshotMoney : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "mvm_mannhattan" );
		m_nCurrencyCollected = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "mvm_sniper_headshot_currency" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "mvm_sniper_headshot_currency" ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				if ( event->GetInt( "userid" ) == pLocalPlayer->GetUserID() )
				{
					m_nCurrencyCollected += event->GetFloat( "currency" );
					if ( m_nCurrencyCollected >= 500 )
					{
						AwardAchievement();
					}
				}
			}
		}
	}

private: 
	int m_nCurrencyCollected;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Sniper_CollectHeadshotMoney, ACHIEVEMENT_TF_MVM_SNIPER_COLLECT_HEADSHOT_MONEY, "TF_MVM_SNIPER_COLLECT_HEADSHOT_MONEY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Medic_ShieldBlockDamage : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "mvm_mannhattan" );
		m_flDamage = 0.0f;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "medigun_shield_blocked_damage" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "teamplay_round_active" ) ||
			 FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			m_flDamage = 0.0f;
		}
		else if ( FStrEq( pszEvent, "medigun_shield_blocked_damage" ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				if ( event->GetInt( "userid" ) == pLocalPlayer->GetUserID() )
				{
					m_flDamage += event->GetFloat( "damage" );
					if ( m_flDamage >= 5000.0f )
					{
						AwardAchievement();
					}
				}
			}
		}
	}

private:
	float m_flDamage;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Medic_ShieldBlockDamage, ACHIEVEMENT_TF_MVM_MEDIC_SHIELD_BLOCK_DAMAGE, "TF_MVM_MEDIC_SHIELD_BLOCK_DAMAGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_Medic_ReviveTeammates : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "revive_player_complete" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "revive_player_complete" ) )
		{
			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
			{
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pLocalPlayer && pLocalPlayer->MedicIsReleasingCharge() )
				{
					if ( event->GetInt( "entindex" ) == GetLocalPlayerIndex() )
					{
						int iNewIndex = m_Times.AddToTail();
						m_Times[iNewIndex] = gpGlobals->curtime;

						// we only care about the last two times we revived someone
						if ( m_Times.Count() > 2 )
						{
							m_Times.Remove( 0 );
						}

						if ( m_Times.Count() == 2 )
						{
							if ( m_Times.Tail() - m_Times.Head() <= 5.0 )
							{
								AwardAchievement();
							}
						}
					}
				}
			}
		}
	}

private:
	CUtlVector< float >	m_Times;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_Medic_ReviveTeammates, ACHIEVEMENT_TF_MVM_MEDIC_REVIVE_TEAMMATES, "TF_MVM_MEDIC_REVIVE_TEAMMATES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_RocketSpecialistKillGrind : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );

		m_flLastDirectTime = 0.f;
		m_Victims.EnsureCapacity( MAX_PLAYERS );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_directhit_stun" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		// If we hit a bot directly, track player deaths over the next 0.25 seconds and see if we're the attacker
		if ( FStrEq( pszEvent, "player_directhit_stun" ) )
		{
			int iLocalPlayerIndex = C_BasePlayer::GetLocalPlayer()->entindex();
			int iAttackerIndex = event->GetInt( "attacker" );
			CBasePlayer *pVictim = UTIL_PlayerByIndex( event->GetInt( "victim" ) );

			if ( pVictim && pVictim->IsPlayer() && iLocalPlayerIndex == iAttackerIndex )
			{
				m_flLastDirectTime = gpGlobals->curtime;
				SetNextThink( 0.1 );
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( gpGlobals->curtime <= m_flLastDirectTime + 0.25f )
		{
			if ( pAttacker && pVictim && pAttacker != pVictim  && pAttacker == C_BasePlayer::GetLocalPlayer() )
			{
				CBasePlayer *pPlayerVictim = UTIL_PlayerByIndex( pVictim->entindex() );
				if ( m_Victims.Find( pPlayerVictim ) == m_Victims.InvalidIndex() )
				{
					m_Victims.AddToTail( pPlayerVictim );
				}
			}
		}
	}

	virtual void Think( void )
	{
		int nVictims = m_Victims.Count();
		if ( nVictims )
		{
			if ( gpGlobals->curtime <= m_flLastDirectTime + 0.25f )
			{
				if ( nVictims >= 5 )
				{
					AwardAchievement();
				}
			}
			else
			{
				m_Victims.RemoveAll();
				return;
			}

			SetNextThink( 0.1 );
		}
	}

private:
	CUtlVector< CBasePlayer* > m_Victims;
	float m_flLastDirectTime;
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_RocketSpecialistKillGrind, ACHIEVEMENT_TF_MVM_ROCKET_SPECIALIST_KILL_GRIND, "TF_MVM_ROCKET_SPECIALIST_KILL_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF_MvM_RocketSpecialistStunGrind : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTF_MvM_RocketSpecialistStunGrind, ACHIEVEMENT_TF_MVM_ROCKET_SPECIALIST_STUN_GRIND, "TF_MVM_ROCKET_SPECIALIST_STUN_GRIND", 5 );

#endif // CLIENT_DLL



