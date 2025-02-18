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
#include "c_tf_objective_resource.h"

//======================================================================================================================================
// SNIPER ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JarateDominated : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Jarate an enemy that you're dominating

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_jarated" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_jarated" ) )
		{
			CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer )
				return;

			int iVictim = event->GetInt( "victim_entindex" );
			if ( pLocalPlayer->m_Shared.IsPlayerDominated(iVictim) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_JarateDominated, ACHIEVEMENT_TF_SNIPER_JARATE_DOMINATED, "TF_SNIPER_JARATE_DOMINATED", 5 );

//----------------------------------------------------------------------------------------------------------------
//Capture the flag as a sniper.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_CaptureTheFlag : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_flag_event" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_flag_event" ) )
		{
			int iCapper = event->GetInt( "player" );
			int iType = event->GetInt( "eventtype" );

			if ( iCapper == GetLocalPlayerIndex() && iType == TF_FLAGEVENT_CAPTURE )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_CaptureTheFlag, ACHIEVEMENT_TF_SNIPER_CAPTURE_FLAG, "TF_SNIPER_CAPTURE_FLAG", 5 );


//----------------------------------------------------------------------------------------------------------------
//Kill an invisible spy.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_InvisibleSpyKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) && pTFVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
			{
				if ( pTFVictim->GetPercentInvisible() == 1.0f )
				{
					IncrementCount(); 
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_InvisibleSpyKill, ACHIEVEMENT_TF_SNIPER_KILL_INVIS_SPY, "TF_SNIPER_KILL_INVIS_SPY", 5 );

//----------------------------------------------------------------------------------------------------------------
//Headshot X enemy snipers
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_HeadShotSnipers: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 10 );		
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) && ( IsHeadshot( event->GetInt( "customkill" ) ) ) && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			IncrementCount();
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_HeadShotSnipers, ACHIEVEMENT_TF_SNIPER_HEADSHOT_SNIPERS, "TF_SNIPER_HEADSHOT_SNIPERS", 5 );

//----------------------------------------------------------------------------------------------------------------
//Headshot an enemy demoman
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_HeadShotDemoman: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) && ( IsHeadshot( event->GetInt( "customkill" ) ) ) && pTFVictim->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			IncrementCount();
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_HeadShotDemoman, ACHIEVEMENT_TF_SNIPER_HEADSHOT_DEMOMAN, "TF_SNIPER_HEADSHOT_DEMOMAN", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill X spies with the kukri
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_SpyKukriKills: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 10 );		
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_SPY) && event->GetInt( "weaponid" ) == TF_WEAPON_CLUB )
			{
				IncrementCount();
			}
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_SpyKukriKills, ACHIEVEMENT_TF_SNIPER_KILL_SPIES_MELEE, "TF_SNIPER_KILL_SPIES_MELEE", 5 );


//----------------------------------------------------------------------------------------------------------------
//Kill a scout in midair
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_ScoutMidairKill: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SCOUT ) && WeaponID_IsSniperRifleOrBow( event->GetInt( "weaponid" ) ) )
			{
				if ( !(pTFVictim->GetFlags() & FL_ONGROUND) )
				{
					IncrementCount();
				}
			}
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_ScoutMidairKill, ACHIEVEMENT_TF_SNIPER_KILL_MIDAIR_SCOUT, "TF_SNIPER_KILL_MIDAIR_SCOUT", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill a fully charged medic
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillChargedMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// client fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillChargedMedic, ACHIEVEMENT_TF_SNIPER_KILL_CHARGED_MEDIC, "TF_SNIPER_KILL_CHARGED_MEDIC", 5 );


//----------------------------------------------------------------------------------------------------------------
//Consolation prize for getting backstabbed
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_Consolation_Backstabs: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 50 );	
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pVictim == C_BasePlayer::GetLocalPlayer() )
		{
			C_TFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

			if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_SPY ) && ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_BACKSTAB ) )
			{
				IncrementCount();
			}
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_Consolation_Backstabs, ACHIEVEMENT_TF_SNIPER_GET_BACKSTABBED, "TF_SNIPER_GET_BACKSTABBED", 5 );

//----------------------------------------------------------------------------------------------------------------
//Dominate an enemy sniper - NOTE: This will probably be iffy if the player you dominated just switched to sniper for that death
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_DominateEnemySniper : public CBaseTFAchievement
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

		if ( pTFVictim && pAttacker == C_TFPlayer::GetLocalTFPlayer() && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) && bDomination == true )
		{
			IncrementCount();
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_DominateEnemySniper, ACHIEVEMENT_TF_SNIPER_DOMINATE_SNIPER, "TF_SNIPER_DOMINATE_SNIPER", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill a rocket jumping soldier/demo
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillRocketJumper : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// client fires an event for this achievement, no other code within achievement necessary
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillRocketJumper, ACHIEVEMENT_TF_SNIPER_KILL_RJER, "TF_SNIPER_KILL_RJER", 5 );

//----------------------------------------------------------------------------------------------------------------
//No scope, bro
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillNoScope : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillNoScope, ACHIEVEMENT_TF_SNIPER_KILL_UNSCOPED, "TF_SNIPER_KILL_UNSCOPED", 5 );


//----------------------------------------------------------------------------------------------------------------
//Kill an enemy while you're dead
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillWhileDead: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pAttacker == C_TFPlayer::GetLocalTFPlayer() && pAttacker->IsAlive() == false )
		{
			IncrementCount();
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillWhileDead, ACHIEVEMENT_TF_SNIPER_BOW_KILL_WHILEDEAD, "TF_SNIPER_BOW_KILL_WHILEDEAD", 5 );

//----------------------------------------------------------------------------------------------------------------
//Extinguish a burning team mate with the jarate
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JarateExtinguish : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_extinguished" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_extinguished" ) )
		{
			CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer )
				return;

			int iTargetIndex = event->GetInt( "victim" );
			int iHealerIndex = event->GetInt( "healer" );

			//Extinguishing myself doesn't count.
			if ( pLocalPlayer->entindex() == iHealerIndex && pLocalPlayer->entindex() != iTargetIndex )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_JarateExtinguish, ACHIEVEMENT_TF_SNIPER_JARATE_EXTINGUISH, "TF_SNIPER_JARATE_EXTINGUISH", 5 );

//----------------------------------------------------------------------------------------------------------------
//Topped the scoreboard
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_TopScoreboard : public CAchievementTopScoreboard
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_TopScoreboard, ACHIEVEMENT_TF_SNIPER_TOP_SCOREBOARD_GRIND, "TF_SNIPER_TOP_SCOREBOARD_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill an opponent within the first second of a round.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillRoundStart: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
		m_flRoundStartTime = -1;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_setup_finished" );
		ListenForGameEvent( "arena_round_start" );
		
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_setup_finished" ) || FStrEq( event->GetName(), "arena_round_start" ) )
		{
			m_flRoundStartTime = gpGlobals->curtime + 1.0f;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			if ( m_flRoundStartTime > 0.0f && m_flRoundStartTime >= gpGlobals->curtime )
			{
				IncrementCount();
			}
		}
	}

private:
	float m_flRoundStartTime;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillRoundStart, ACHIEVEMENT_TF_SNIPER_KILL_AT_ROUNDSTART, "TF_SNIPER_KILL_AT_ROUNDSTART", 5 );

class CAchievementTFSniper_FreezecamHat : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_FreezecamHat, ACHIEVEMENT_TF_SNIPER_FREEZECAM_HAT, "TF_SNIPER_FREEZECAM_HAT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_FreezecamWave : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_FreezecamWave, ACHIEVEMENT_TF_SNIPER_FREEZECAM_WAVE, "TF_SNIPER_FREEZECAM_WAVE", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill an opponent with 3 different weapons in one round
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_DifferentWeaponsKill: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
		m_Weapons.Purge();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_Weapons.Purge();
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			int iWeaponID = event->GetInt( "weaponid" );
			
			if ( m_Weapons.Find( iWeaponID ) == -1 )
			{
				m_Weapons.AddToTail( iWeaponID );

				if ( m_Weapons.Count() == 3 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	CUtlVector< int >	m_Weapons;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_DifferentWeaponsKill, ACHIEVEMENT_TF_SNIPER_KILL_WEAPONS, "TF_SNIPER_KILL_WEAPONS", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill grind
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillEnemyGrind: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1000 );		
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() && pVictim && pVictim->IsPlayer() )
		{
			IncrementCount();
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillEnemyGrind, ACHIEVEMENT_TF_SNIPER_KILL_GRIND, "TF_SNIPER_KILL_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
//Destroy 3 enemy sentries
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_DestroySentry : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "object_destroyed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "object_destroyed" ) == 0 )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		
			if ( UTIL_PlayerByIndex( iIndex ) == C_TFPlayer::GetLocalTFPlayer() )
			{
				if ( event->GetInt( "objecttype" ) == OBJ_SENTRYGUN )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_DestroySentry, ACHIEVEMENT_TF_SNIPER_DESTROY_SENTRYGUNS, "TF_SNIPER_DESTROY_SENTRYGUNS", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill enemies doing important stuff.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillEnemiesImportant : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iObjectivesDefended = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_capture_blocked" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iObjectivesDefended = 0;
			return;
		}

		if ( FStrEq( event->GetName(), "teamplay_capture_blocked" ) )
		{
			int iBlocker = event->GetInt( "blocker" );

			if ( iBlocker == GetLocalPlayerIndex() )
			{
				m_iObjectivesDefended++;
			}
		}

		if ( FStrEq( event->GetName(), "teamplay_flag_event" ) && event->GetInt( "eventtype" ) == TF_FLAGEVENT_DEFEND )
		{
			int iIndex = event->GetInt( "player" );

			if ( GetLocalPlayerIndex() == iIndex )
			{
				m_iObjectivesDefended++;
			}
		}

		if ( m_iObjectivesDefended >= 3 )
		{
			IncrementCount();
		}
	}

private:
	int m_iObjectivesDefended;
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillEnemiesImportant, ACHIEVEMENT_TF_SNIPER_KILL_OBJECTIVES, "TF_SNIPER_KILL_OBJECTIVES", 5 );


//----------------------------------------------------------------------------------------------------------------
//Kill someone the moment they leave invul
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_HeadShotPostInvuln: public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() && pTFVictim && IsHeadshot ( event->GetInt( "customkill" ) ) )
		{
			if ( pTFVictim->m_Shared.GetInvulOffTime() > 0.0f && (gpGlobals->curtime - pTFVictim->m_Shared.GetInvulOffTime() ) <= 1.0f )
			{
				IncrementCount();
			}
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_HeadShotPostInvuln, ACHIEVEMENT_TF_SNIPER_HEADSHOT_POST_INVULN, "TF_SNIPER_HEADSHOT_POST_INVULN", 5 );

//----------------------------------------------------------------------------------------------------------------
//Baseclass for jarate/group related stuff
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_BaseJarateGroup : public CBaseTFAchievement
{

public:

	DECLARE_CLASS( CAchievementTFSniper_BaseJarateGroup, CBaseTFAchievement );

	void Init() 
	{
		m_flJarateTime = 0.0f;
		m_JaratedPlayers.Purge();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_jarated" );
		ListenForGameEvent( "player_jarated_fade" );
	}

	virtual void AddedJaratedPlayer( void )
	{

	}

	virtual void RemoveJaratedPlayer( void )
	{

	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_jarated_fade" ) )
		{
			int iThrower = event->GetInt( "thrower_entindex" );
			int iVictim = event->GetInt( "victim_entindex" );

			if ( GetLocalPlayerIndex() != iThrower )
				return;

			m_JaratedPlayers.FindAndRemove( iVictim );

			RemoveJaratedPlayer();
		}
		else if ( FStrEq( event->GetName(), "player_jarated" ) )
		{
			int iThrower = event->GetInt( "thrower_entindex" );
			int iVictim = event->GetInt( "victim_entindex" );

			if ( GetLocalPlayerIndex() != iThrower )
				return;

			if ( m_flJarateTime <= gpGlobals->curtime )
			{
				m_flJarateTime = gpGlobals->curtime + 0.5;
				m_JaratedPlayers.Purge();
			}

			if ( m_JaratedPlayers.Find( iVictim ) == m_JaratedPlayers.InvalidIndex() )
			{
				m_JaratedPlayers.AddToTail( iVictim );
			}

			AddedJaratedPlayer();
		}
	}

	CUtlVector< int > m_JaratedPlayers;
	float m_flJarateTime;
};

//----------------------------------------------------------------------------------------------------------------
//Jarate 4 players with the same jar.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JaratePack : public CAchievementTFSniper_BaseJarateGroup
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_flJarateTime = 0.0f;
		m_JaratedPlayers.Purge();
	}

	virtual void AddedJaratedPlayer( void )
	{
		if ( m_JaratedPlayers.Count() == 4 )
		{
			IncrementCount();
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_JaratePack, ACHIEVEMENT_TF_SNIPER_JARATE_GROUP, "TF_SNIPER_JARATE_GROUP", 5 );


//----------------------------------------------------------------------------------------------------------------
//Jarate a medic and his heal target
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JarateMedicPair : public CAchievementTFSniper_BaseJarateGroup
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_flJarateTime = 0.0f;
		m_JaratedPlayers.Purge();
	}

	virtual void AddedJaratedPlayer( void )
	{
		for ( int i = 0; i < m_JaratedPlayers.Count(); i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( m_JaratedPlayers[i] ) );

			if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				CTFPlayer *pHealTarget = ToTFPlayer( pPlayer->MedicGetHealTarget() );

				if ( pHealTarget && m_JaratedPlayers.Find( pHealTarget->entindex() ) != -1 )
				{
					IncrementCount();
				}
			}
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_JarateMedicPair, ACHIEVEMENT_TF_SNIPER_JARATE_MEDIC_PAIR, "TF_SNIPER_JARATE_MEDIC_PAIR", 5 );


//----------------------------------------------------------------------------------------------------------------
//Jarated an invisible spy
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JarateReveal : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_JarateReveal, ACHIEVEMENT_TF_SNIPER_JARATE_REVEAL_SPY, "TF_SNIPER_JARATE_REVEAL_SPY", 5 );

//----------------------------------------------------------------------------------------------------------------
//Pin heavy by the head
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_PinHeavy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );	
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_pinned" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_pinned" ) )
		{
			IncrementCount();	
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_PinHeavy, ACHIEVEMENT_TF_SNIPER_BOW_PIN_HEAVY, "TF_SNIPER_BOW_PIN_HEAVY", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill 3 jarated enemies with your kukri
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JarateKillMelee : public CAchievementTFSniper_BaseJarateGroup
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );	
		m_JaratedPlayers.Purge();
		m_iKilled = 0;
	}

	void RemoveJaratedPlayer( void )
	{
		if ( m_JaratedPlayers.Count() == 0 )
		{
			m_iKilled = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pVictim && pAttacker == C_TFPlayer::GetLocalTFPlayer() && event->GetInt( "weaponid" ) == TF_WEAPON_CLUB )
		{
			if ( m_JaratedPlayers.Find( pVictim->entindex() ) != m_JaratedPlayers.InvalidIndex() )
			{
				m_iKilled++;

				if ( m_iKilled == 3 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	int m_iKilled;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_JarateKillMelee, ACHIEVEMENT_TF_SNIPER_JARATE_KILL_MELEE, "TF_SNIPER_JARATE_KILL_MELEE", 5 );


//----------------------------------------------------------------------------------------------------------------
//Kill a spy that just failed a backstab because of the shield
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_ShieldFailedSpy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );	

		m_iAttackerIndex = 0;
		m_flAttackTime = 0.0f;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_shield_blocked" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_shield_blocked" ) )
		{
			int iAttacker = event->GetInt( "attacker_entindex" );
			int iBlocker = event->GetInt( "blocker_entindex" );

			if ( GetLocalPlayerIndex() != iBlocker )
				return;

			m_iAttackerIndex = iAttacker;
			m_flAttackTime = gpGlobals->curtime + 10.0f;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( m_flAttackTime <= gpGlobals->curtime )
		{
			m_iAttackerIndex = 0;
		}

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pTFVictim->entindex() == m_iAttackerIndex && pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			IncrementCount();
		}
	}

private:
	int m_iAttackerIndex;
	float m_flAttackTime;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_ShieldFailedSpy, ACHIEVEMENT_TF_SNIPER_KILL_FAILED_SPY, "TF_SNIPER_KILL_FAILED_SPY", 5 );


//----------------------------------------------------------------------------------------------------------------
// Kill a medic/heavy pair with the bow.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillMedicPair : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL  | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );

		m_hTargets.Purge();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			m_hTargets.Purge();
		}
	}

	int	GetTargetIndex( CBaseEntity *pTarget )
	{
		for ( int i = 0; i < m_hTargets.Count(); i++ )
		{
			if ( m_hTargets[i].hTarget == pTarget )
				return i;
		}
		return -1;
	}

	void AddNewTarget( CBaseEntity *pTarget )
	{
		if ( !pTarget )
			return;

		// see if the target is already in our list or get a new index
		int iTargetIndex = GetTargetIndex( pTarget );
		if ( iTargetIndex == -1 )
		{
			iTargetIndex = m_hTargets.AddToTail();
		}

		m_hTargets[iTargetIndex].hTarget = pTarget;
		m_hTargets[iTargetIndex].flTimeToBeat = gpGlobals->curtime + 15.0f; // 15 seconds to kill the target
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pLocalPlayer == pAttacker && event->GetInt( "weaponid" ) == TF_WEAPON_COMPOUND_BOW )
		{
			// is this victim in our list of targets?
			int index = GetTargetIndex( pVictim );
			if ( index != -1 )
			{
				// did we beat the time?
				if ( m_hTargets[index].flTimeToBeat > gpGlobals->curtime )
				{
					IncrementCount();
				}
			}
			else
			{
				C_TFPlayer *pNewTarget = NULL;
				C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
				{
					for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
					{
						pNewTarget = ToTFPlayer( UTIL_PlayerByIndex( i ) );
						if ( pNewTarget && pNewTarget->IsPlayerClass( TF_CLASS_MEDIC ) && pNewTarget->MedicGetHealTarget() == pTFVictim )
						{
							// add all of his Medics to our list of targets (could be more than one Medic)
							AddNewTarget( pNewTarget );
						}
					}
				}
				else if ( pTFVictim->IsPlayerClass( TF_CLASS_MEDIC ) )
				{
					pNewTarget = ToTFPlayer( pTFVictim->MedicGetHealTarget() );
					if ( pNewTarget && pNewTarget->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
					{	
						AddNewTarget( pNewTarget );
					}
				}
			}
		}

		// is this victim in our list of targets?
		int index_ = GetTargetIndex( pVictim );
		if ( index_ != -1 )
		{
			m_hTargets.Remove( index_ );
		}
	}

private:
	struct targets_t
	{
		EHANDLE hTarget;
		float flTimeToBeat;
	};

	CUtlVector<targets_t> m_hTargets;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillMedicPair, ACHIEVEMENT_TF_SNIPER_BOW_KILL_MEDIC_PAIR, "TF_SNIPER_BOW_KILL_MEDIC_PAIR", 5 );


//----------------------------------------------------------------------------------------------------------------
//Kill 3 jarated enemies with your kukri
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_JarateAssists : public CAchievementTFSniper_BaseJarateGroup
{
	DECLARE_CLASS( CAchievementTFSniper_JarateAssists, CAchievementTFSniper_BaseJarateGroup );

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );	
		m_JaratedPlayers.Purge();
		m_iAssists = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_round_active" );

		BaseClass::ListenForEvents();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_JaratedPlayers.Purge();
			m_iAssists = 0;
		}

		BaseClass::FireGameEvent_Internal( event );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );

		if ( iAssisterIndex > 0 )
		{
			C_TFPlayer *pTFAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );

			if ( pVictim && pTFAssister == C_TFPlayer::GetLocalTFPlayer() )
			{
				if ( m_JaratedPlayers.Find( pVictim->entindex() ) != m_JaratedPlayers.InvalidIndex() )
				{
					m_iAssists++;

					if ( m_iAssists == 5 )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	int m_iAssists;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_JarateAssists, ACHIEVEMENT_TF_SNIPER_JARATE_ASSISTS, "TF_SNIPER_JARATE_ASSISTS", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill an enemy carrying your flag with the bow
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_KillFlagCarrierBow : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_KillFlagCarrierBow, ACHIEVEMENT_TF_SNIPER_BOW_KILL_FLAGCARRIER, "TF_SNIPER_BOW_KILL_FLAGCARRIER", 5 );

//----------------------------------------------------------------------------------------------------------------
//Boromir someone
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_Pincushion : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );	
		m_hTargets.Purge();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "arrow_impact" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) || FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			 m_hTargets.Purge();
		}
		else if ( FStrEq( event->GetName(), "arrow_impact" ) )
		{
			int iEntity = event->GetInt( "attachedEntity" );
			int iShooter = event->GetInt( "shooter" );

			if ( iShooter == GetLocalPlayerIndex() )
			{
				int iTargetIndex = GetTargetIndex( UTIL_PlayerByIndex( iEntity ) );

				if ( iTargetIndex == -1 )
				{
					AddNewTarget( UTIL_PlayerByIndex( iEntity ) );
				}
				else
				{
					 m_hTargets[iTargetIndex].iArrows++;

					 if ( m_hTargets[iTargetIndex].iArrows >= 3 )
					 {
						 IncrementCount();
						 m_hTargets.Purge();
					 }
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			int iTargetIndex = GetTargetIndex( pVictim );

			if ( iTargetIndex != -1 )
			{
				m_hTargets.Remove( iTargetIndex );
			}
		}
	}

	int	GetTargetIndex( CBaseEntity *pTarget )
	{
		for ( int i = 0; i < m_hTargets.Count(); i++ )
		{
			if ( m_hTargets[i].hTarget == pTarget )
				return i;
		}
		return -1;
	}

	void AddNewTarget( CBaseEntity *pTarget )
	{
		if ( !pTarget )
			return;

		if ( pTarget->IsAlive() == false )
			return;

		// see if the target is already in our list or get a new index
		int iTargetIndex = GetTargetIndex( pTarget );
		if ( iTargetIndex == -1 )
		{
			iTargetIndex = m_hTargets.AddToTail();
		}

		m_hTargets[iTargetIndex].hTarget = pTarget;
		m_hTargets[iTargetIndex].iArrows = 1;
	}

private:
	struct targets_t
	{
		EHANDLE hTarget;
		int iArrows;
		float flRemoveTime;
	};

	CUtlVector<targets_t> m_hTargets;
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_Pincushion, ACHIEVEMENT_TF_SNIPER_BOW_PINCUSHION, "TF_SNIPER_BOW_PINCUSHION", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_SniperTauntKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			// we already know we killed a player because of the ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS flag
			// was this a taunt kill?
			if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB )
			{
				IncrementCount();
			}
		}
	}
};

DECLARE_ACHIEVEMENT( CAchievementTFSniper_SniperTauntKill, ACHIEVEMENT_TF_SNIPER_TAUNT_KILL, "TF_SNIPER_TAUNT_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_SniperRifleNoMissing : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_SniperRifleNoMissing, ACHIEVEMENT_TF_SNIPER_RIFLE_NO_MISSING, "TF_SNIPER_RIFLE_NO_MISSING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSniper_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 5, ACHIEVEMENT_TF_SNIPER_START_RANGE, ACHIEVEMENT_TF_SNIPER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_AchieveProgress1, ACHIEVEMENT_TF_SNIPER_ACHIEVE_PROGRESS1, "TF_SNIPER_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSniper_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 11, ACHIEVEMENT_TF_SNIPER_START_RANGE, ACHIEVEMENT_TF_SNIPER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_AchieveProgress2, ACHIEVEMENT_TF_SNIPER_ACHIEVE_PROGRESS2, "TF_SNIPER_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSniper_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 17, ACHIEVEMENT_TF_SNIPER_START_RANGE, ACHIEVEMENT_TF_SNIPER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_AchieveProgress3, ACHIEVEMENT_TF_SNIPER_ACHIEVE_PROGRESS3, "TF_SNIPER_ACHIEVE_PROGRESS3", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_ClassicRifleNoScopeHeadshot : public CBaseTFAchievement
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
DECLARE_ACHIEVEMENT( CAchievementTFSniper_ClassicRifleNoScopeHeadshot, ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_NOSCOPE_HEADSHOT, "TF_SNIPER_CLASSIC_RIFLE_NOSCOPE_HEADSHOT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_ClassicRifleHeadshotJumper : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_ClassicRifleHeadshotJumper, ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_HEADSHOT_JUMPER, "TF_SNIPER_CLASSIC_RIFLE_HEADSHOT_JUMPER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_ClassicRifleGibGrind : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iClassesKilled = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_iClassesKilled = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim && ( pAttacker == CBasePlayer::GetLocalPlayer() ) )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_SNIPERRIFLE_CLASSIC )
			{
				if ( event->GetInt( "death_flags" ) & TF_DEATH_GIBBED )
				{
					CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
					if ( pTFVictim && pTFVictim->GetPlayerClass() )
					{
						int iClass = pTFVictim->GetPlayerClass()->GetClassIndex();
						if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= ( TF_LAST_NORMAL_CLASS - 1 ) ) //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
						{
							// yes, the achievement is satisfied for this class, set the corresponding bit
							int iBitNumber = ( iClass - TF_FIRST_NORMAL_CLASS );
							m_iClassesKilled |= ( 1 << iBitNumber );

							if ( m_iClassesKilled == 511 )
							{
								IncrementCount();
							}
						}
					}
				}
			}
		}
	}

private:
	int m_iClassesKilled;
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_ClassicRifleGibGrind, ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_GIB_GRIND, "TF_SNIPER_CLASSIC_RIFLE_GIB_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSniper_ParachuteGrind : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim && ( pAttacker == CBasePlayer::GetLocalPlayer() ) )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && 
				( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) && 
				( IsHeadshot( event->GetInt( "customkill" ) ) ) && 
				( event->GetInt( "damagebits" ) & DMG_CRITICAL ) && 
				( pTFVictim->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSniper_ParachuteGrind, ACHIEVEMENT_TF_SNIPER_PARACHUTE_GRIND, "TF_SNIPER_PARACHUTE_GRIND", 5 );

#endif // CLIENT_DLL
