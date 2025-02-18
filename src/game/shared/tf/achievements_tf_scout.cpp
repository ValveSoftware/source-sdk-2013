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
#include "tf_weapon_shotgun.h"
#include "usermessages.h"

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_FirstBlood : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for getting first blood.
		if ( event->GetInt( "death_flags" ) & TF_DEATH_FIRST_BLOOD )
		{
			IncrementCount();
		}
	}

};
DECLARE_ACHIEVEMENT( CAchievementTFScout_FirstBlood, ACHIEVEMENT_TF_SCOUT_FIRST_BLOOD, "TF_SCOUT_FIRST_BLOOD", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_FirstBloodKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pAttackerPlayer = ToTFPlayer( pAttacker );

		// Achievement for accumulating 5 kills over several matches with the first blood crit boost.
		bool bFirstBlood = event->GetInt( "death_flags" ) & TF_DEATH_FIRST_BLOOD;
		if ( pAttackerPlayer->m_Shared.IsFirstBloodBoosted() &&
			 pAttackerPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED_FIRST_BLOOD ) &&
			 !bFirstBlood )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_FirstBloodKill, ACHIEVEMENT_TF_SCOUT_FIRST_BLOOD_KILL, "TF_SCOUT_FIRST_BLOOD_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_WellEarlyKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for getting a kill in cp_well during the setup phase.
		if ( FStrEq( m_pAchievementMgr->GetMapName(), "cp_well" ) &&
			 TFGameRules() && (TFGameRules()->State_Get() == GR_STATE_RND_RUNNING) &&
			 TFGameRules()->InSetup() )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_WellEarlyKill, ACHIEVEMENT_TF_SCOUT_WELL_EARLY_KILL, "TF_SCOUT_WELL_EARLY_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_LifetimeKills : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 2004 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for getting 2004 lifetime kills!
		IncrementCount();
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_LifetimeKills, ACHIEVEMENT_TF_SCOUT_LIFETIME_KILLS, "TF_SCOUT_LIFETIME_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_IronManKills : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		ResetTracking();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );

		ResetTracking();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			ResetTracking();
		}
	}
	
	void ResetTracking()
	{
		m_bKillOnGround = false;
		m_bKillInAir = false;
		m_bKillInWater = false;

	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for getting a kill while on the ground, in the air, and in the water in a single life.
		CTFPlayer *pAttackerPlayer = ToTFPlayer( pAttacker );
		if ( !pAttackerPlayer )
			return;

		if ( pAttackerPlayer->GetWaterLevel() == WL_Eyes )
		{
			m_bKillInWater = true;
		}
		else if ( !(pAttackerPlayer->GetFlags() & FL_ONGROUND) )
		{
			m_bKillInAir = true;
		}
		else
		{
			m_bKillOnGround = true;
		}

		if ( m_bKillOnGround && m_bKillInAir && m_bKillInWater )
		{
			IncrementCount();
		}
	}
	
	bool m_bKillOnGround;
	bool m_bKillInAir;
	bool m_bKillInWater;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_IronManKills, ACHIEVEMENT_TF_SCOUT_IRON_MAN_KILLS, "TF_SCOUT_IRON_MAN_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_DestroyTeleporters : public CBaseTFAchievement
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
			CBaseEntity *pDestroyer = UTIL_PlayerByIndex( iIndex );
			C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pDestroyer == pTFPlayer )
			{
				// Only count teleporters.
				int iType = event->GetInt( "objecttype" );
				if ( iType == OBJ_TELEPORTER )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_DestroyTeleporters, ACHIEVEMENT_TF_SCOUT_DESTROY_TELEPORTERS, "TF_SCOUT_DESTROY_TELEPORTERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_DestroyBuildingsBeingBuilt : public CBaseTFAchievement
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
			CBaseEntity *pDestroyer = UTIL_PlayerByIndex( iIndex );
			C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pDestroyer == pTFPlayer )
			{
				// Only count buildings being built.
				if ( event->GetBool( "was_building" ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_DestroyBuildingsBeingBuilt, ACHIEVEMENT_TF_SCOUT_DESTROY_BUILDINGS_BEING_BUILT, "TF_SCOUT_DESTROY_BUILDINGS_BEING_BUILT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_DestroySentryWithPistol : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
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
			CBaseEntity *pDestroyer = UTIL_PlayerByIndex( iIndex );
			C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pDestroyer == pTFPlayer )
			{
				// Only count active sentries destroyed with a pistol.
				int iType = event->GetInt( "objecttype" );
				int iWeaponID = event->GetInt( "weaponid" );
				if ( (iType == OBJ_SENTRYGUN) &&
					( (iWeaponID == TF_WEAPON_PISTOL_SCOUT) || (iWeaponID == TF_WEAPON_HANDGUN_SCOUT_SECONDARY) ) &&
					!event->GetBool( "was_building" ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_DestroySentryWithPistol, ACHIEVEMENT_TF_SCOUT_DESTROY_SENTRY_WITH_PISTOL, "TF_SCOUT_DESTROY_SENTRY_WITH_PISTOL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_DoubleJumps : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000 );
		SetStoreProgressInSteam( true );

		m_bScoredJump = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "air_dash" );
		ListenForGameEvent( "landed" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// Achievement for getting 1000 double jumps.
		if ( Q_strcmp( event->GetName(), "air_dash" ) == 0 )
		{
			int iJumperID = event->GetInt( "player" );
			C_TFPlayer *pJumper = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( iJumperID ) ) );
			if ( pJumper == C_TFPlayer::GetLocalTFPlayer() && !m_bScoredJump )
			{
				m_bScoredJump = true;
				IncrementCount();
			}
		}
		else if ( (Q_strcmp( event->GetName(), "landed" ) == 0) ||
		          (Q_strcmp( event->GetName(), "localplayer_respawn" ) == 0) ||
				  (Q_strcmp( event->GetName(), "teamplay_round_active" ) == 0) )
		{
			m_bScoredJump = false;
		}
	}

private:
	bool	m_bScoredJump;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_DoubleJumps, ACHIEVEMENT_TF_SCOUT_DOUBLE_JUMPS, "TF_SCOUT_DOUBLE_JUMPS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_AssistChargeMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		m_bCharged = false;
		m_iAssists = 0;
		m_iMedic = -1;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_chargedeployed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_chargedeployed" ) == 0 )
		{
			m_iMedic = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pMedic = UTIL_PlayerByIndex( m_iMedic );
			int iTarget = engine->GetPlayerForUserID( event->GetInt( "targetid" ) );
			CBaseEntity *pTarget = UTIL_PlayerByIndex( iTarget );

			if ( pMedic && pTarget && pTarget == C_TFPlayer::GetLocalTFPlayer() )
			{
				m_bCharged = true;
				m_iAssists = 0;
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		if ( iAssisterIndex > 0 && iAssisterIndex == m_iMedic )
		{
			C_TFPlayer *pMedic = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
			if ( pMedic && pMedic->MedicIsReleasingCharge() )
			{
				m_iAssists++;
				if ( m_iAssists >= 3 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	bool	m_bCharged;
	int		m_iAssists;
	int		m_iMedic;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_AssistChargeMedic, ACHIEVEMENT_TF_SCOUT_ASSIST_MEDIC, "TF_SCOUT_ASSIST_MEDIC", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_StealSandwich : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		m_iHeavyID = -1;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_stealsandvich" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();
		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			// If we died or respawned, reset...
			m_iHeavyID = -1;
			return;
		}
		else if ( Q_strcmp( event->GetName(), "player_stealsandvich" ) == 0 )
		{
			int iOwner = engine->GetPlayerForUserID( event->GetInt( "owner" ) );
			if ( iOwner != m_iHeavyID )
				return;

			int iTarget = engine->GetPlayerForUserID( event->GetInt( "target" ) );
			CBaseEntity *pTarget = UTIL_PlayerByIndex( iTarget );

			if ( pTarget && pTarget == C_TFPlayer::GetLocalTFPlayer() )
			{
				IncrementCount();
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for killing a Heavy and taking his sandvich.
		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			// Track the heavy: we have to be responsible for his death.
			m_iHeavyID = engine->GetPlayerForUserID( pTFVictim->GetUserID() );
		}
	}

private:
	int		m_iHeavyID;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_StealSandwich, ACHIEVEMENT_TF_SCOUT_STEAL_SANDWICH, "TF_SCOUT_STEAL_SANDWICH", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_KillChargedMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// client fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_KillChargedMedic, ACHIEVEMENT_TF_SCOUT_KILL_CHARGED_MEDICS, "TF_SCOUT_KILL_CHARGED_MEDICS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_SurviveDamage : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_damaged" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_iDamageTotal = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_damaged" ) == 0 )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

			m_iDamageTotal += event->GetInt( "amount" );

			if ( pLocalPlayer && pLocalPlayer->IsAlive() && m_iDamageTotal >= 500 )
			{
				IncrementCount();
			}
		}
		else if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_iDamageTotal = 0;
		}
		else if ( FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iDamageTotal = 0;
		}
	}

private:
	int		m_iDamageTotal;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_SurviveDamage, ACHIEVEMENT_TF_SCOUT_SURVIVE_DAMAGE, "TF_SCOUT_SURVIVE_DAMAGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_ThreeFlagCaptures : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "teamplay_round_active" );
		m_iFlagCaps = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "teamplay_flag_event" ) == 0 && event->GetInt( "eventtype" ) == TF_FLAGEVENT_CAPTURE )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

			if ( pLocalPlayer && pLocalPlayer->entindex() == event->GetInt( "player" ))
			{
				m_iFlagCaps++;

				if ( m_iFlagCaps >= 3 )
				{
					IncrementCount();
				}
			}
		}
		else if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_iFlagCaps = 0;
		}
	}

private:
	int		m_iFlagCaps;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_ThreeFlagCaptures, ACHIEVEMENT_TF_SCOUT_THREE_FLAGCAPS, "TF_SCOUT_THREE_FLAGCAPS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_DoubleJumpKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pKiller = ToTFPlayer( pAttacker );
		int iDoubleJumpKill = pKiller->m_Shared.GetAirDash();

		if ( iDoubleJumpKill > 0 )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_DoubleJumpKill, ACHIEVEMENT_TF_SCOUT_DOUBLEJUMP_KILL, "TF_SCOUT_DOUBLEJUMP_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_FlagCapGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_flag_event" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "teamplay_flag_event" ) == 0 && event->GetInt( "eventtype" ) == TF_FLAGEVENT_CAPTURE )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

			if ( pLocalPlayer && pLocalPlayer->entindex() == event->GetInt( "player" ))
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_FlagCapGrind, ACHIEVEMENT_TF_SCOUT_FLAG_CAP_GRIND, "TF_SCOUT_FLAG_CAP_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_DodgeDamage : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iDamageDodged = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_damage_dodged" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();
		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			// If we died or respawned, reset...
			m_iDamageDodged = 0;
			return;
		}
		else if ( Q_strcmp( event->GetName(), "player_damage_dodged" ) == 0 )
		{
			m_iDamageDodged += event->GetInt( "damage" );

			if ( m_iDamageDodged > 1000 )
			{
				IncrementCount();
			}
		}
	}

private:
	int		m_iDamageDodged;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_DodgeDamage, ACHIEVEMENT_TF_SCOUT_DODGE_DAMAGE, "TF_SCOUT_DODGE_DAMAGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_KnockIntoTrain : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CBasePlayer *pLocalPlayer = C_TFPlayer::GetLocalPlayer();
		if ( !pLocalPlayer )
			return;

		if ( pTFVictim->m_Shared.GetWeaponKnockbackID() == pLocalPlayer->GetUserID() )
		{
			int custom = event->GetInt( "customkill" );
			int damagebits = event->GetInt( "damagebits" );
			if ( ( damagebits & DMG_VEHICLE ) || // They were hit by a freakin' train!
				 ( pAttacker && pAttacker->IsBrushModel() ) || // They were smashed by the world! Gah!
				 ( !pAttacker || (pAttacker == pVictim) ) || // He killed himself!
				 ( custom == TF_DMG_CUSTOM_SUICIDE ) ||
				 ( custom == TF_DMG_CUSTOM_TRIGGER_HURT ) ||  // A trigger-hurt got him! 
				 ( custom == TF_DMG_CUSTOM_CROC ) ) // a croc got him!
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_KnockIntoTrain, ACHIEVEMENT_TF_SCOUT_KNOCK_INTO_TRAIN, "TF_SCOUT_KNOCK_INTO_TRAIN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_KillStunned : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CBasePlayer *pLocalPlayer = C_TFPlayer::GetLocalPlayer();
		if ( !pLocalPlayer )
			return;

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		if ( pTFVictim->m_Shared.InCond( TF_COND_STUNNED ) )
		{
			if ( pAttacker != pLocalPlayer )
			{
				int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
				if ( iAssisterIndex <= 0 )
					return;

				CTFPlayer *pAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
				if ( pAssister != pLocalPlayer )
					return;
			}

			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_KillStunned, ACHIEVEMENT_TF_SCOUT_KILL_STUNNED, "TF_SCOUT_KILL_STUNNED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_StunIntoTrain : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		// Achievement for causing someone we stunned to die by the environment.
		CTFPlayer *pLocalTFPlayer =  C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalTFPlayer )
			return;

		if ( pTFVictim->m_Shared.GetStunner() == pLocalTFPlayer && pTFVictim->m_Shared.InCond( TF_COND_STUNNED ) )
		{
			int custom = event->GetInt( "customkill" );
			int damagebits = event->GetInt( "damagebits" );
			if ( ( damagebits & DMG_VEHICLE ) || // They were hit by a freakin' train!
				( pAttacker && pAttacker->IsBrushModel() ) || // They were smashed by the world! Gah!
				( !pAttacker || (pAttacker == pVictim) ) || // He killed himself!
				( custom == TF_DMG_CUSTOM_SUICIDE ) ||
				( custom == TF_DMG_CUSTOM_TRIGGER_HURT ) ||  // A trigger-hurt got him! 
				( custom == TF_DMG_CUSTOM_CROC ) ) // a croc got him!
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_StunIntoTrain, ACHIEVEMENT_TF_SCOUT_STUN_INTO_TRAIN, "TF_SCOUT_STUN_INTO_TRAIN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_StunUberEnemies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 2 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_stunned" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "player_stunned" ) )
		{
			CTFPlayer *pStunner = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "stunner" ) ) ) );
			CTFPlayer *pVictim  = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "victim" ) ) ) );

			if ( pStunner == pLocalPlayer )
			{
				if ( pVictim && pVictim->IsPlayerClass( TF_CLASS_MEDIC ) && pVictim->MedicGetChargeLevel() >= 1.0 )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_StunUberEnemies, ACHIEVEMENT_TF_SCOUT_STUN_UBER_ENEMIES, "TF_SCOUT_STUN_UBER_ENEMIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_StunCappingEnemies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_stunned" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "player_stunned" ) )
		{
			CTFPlayer *pStunner = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "stunner" ) ) ) );
			CTFPlayer *pVictim  = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "victim" ) ) ) );

			if ( (pStunner == pLocalPlayer) &&
				pVictim && event->GetBool( "victim_capping" ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_StunCappingEnemies, ACHIEVEMENT_TF_SCOUT_STUN_CAPPING_ENEMIES, "TF_SCOUT_STUN_CAPPING_ENEMIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_MaxStuns : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_stunned" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "player_stunned" ) )
		{
			CTFPlayer *pStunner = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "stunner" ) ) ) );
			CTFPlayer *pVictim  = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "victim" ) ) ) );

			if ( (pStunner == pLocalPlayer) &&
				 pVictim && event->GetBool( "big_stun" ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_MaxStuns, ACHIEVEMENT_TF_SCOUT_MAX_STUNS, "TF_SCOUT_MAX_STUNS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_StunScoutWithTheirBall : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_StunScoutWithTheirBall, ACHIEVEMENT_TF_SCOUT_STUN_SCOUT_WITH_THEIR_BALL, "TF_SCOUT_STUN_SCOUT_WITH_THEIR_BALL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_KillInDodgeCooldown : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for killing another scout under the effect Crit-a-Cola
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SCOUT ) && pTFVictim->m_Shared.InCond( TF_COND_ENERGY_BUFF ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_KillInDodgeCooldown, ACHIEVEMENT_TF_SCOUT_KILL_IN_DODGE_COOLDOWN, "TF_SCOUT_KILL_IN_DODGE_COOLDOWN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_KillFromBehind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// Shamelessly stolen from the knife!
	float DotProductToTarget( CBaseEntity *pAttacker, CBaseEntity *pTarget )
	{
		Assert( pTarget );

		// Get the forward view vector of the target, ignore Z
		Vector vecVictimForward;
		AngleVectors( pTarget->EyeAngles(), &vecVictimForward, NULL, NULL );
		vecVictimForward.z = 0.0f;
		vecVictimForward.NormalizeInPlace();

		// Get a vector from my origin to my targets origin
		Vector vecToTarget;
		vecToTarget = pTarget->WorldSpaceCenter() - pAttacker->WorldSpaceCenter();
		vecToTarget.z = 0.0f;
		vecToTarget.NormalizeInPlace();

		return DotProduct( vecVictimForward, vecToTarget );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for killing a bunch of enemies from behind with the scattergun double.
		if ( event->GetInt( "weaponid" ) == TF_WEAPON_SCATTERGUN )
		{
			CTFScatterGun *pScattergun = (CTFScatterGun *)ToTFPlayer( C_TFPlayer::GetLocalPlayer() )->Weapon_OwnsThisID( TF_WEAPON_SCATTERGUN );
			if ( pScattergun && pScattergun->HasKnockback() &&
				( DotProductToTarget( pAttacker, pVictim ) > -0.1 ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_KillFromBehind, ACHIEVEMENT_TF_SCOUT_KILL_FROM_BEHIND, "TF_SCOUT_KILL_FROM_BEHIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_CaptureLastPoint : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_bRecentCapper = false;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			m_bRecentCapper = false;
			int iTeam = event->GetInt( "team" );
			if ( iTeam == pLocalPlayer->GetTeamNumber() )
			{
				const char *cappers = event->GetString( "cappers" );
				for ( int i=0; i<Q_strlen( cappers ); i++ )
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
			if ( (iTeam == pLocalPlayer->GetTeamNumber()) &&
				 m_bRecentCapper &&
				 (TFGameRules()->GetGameType() == TF_GAMETYPE_CP) )
			{
				IncrementCount();
			}
			m_bRecentCapper = false;
		}
	}

	bool m_bRecentCapper;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_CaptureLastPoint, ACHIEVEMENT_TF_SCOUT_CAPTURE_LAST_POINT, "TF_SCOUT_CAPTURE_LAST_POINT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_CaptureThreePoints : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_iPointsCapped = 0;
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// only on maps with capture points
		if ( TFGameRules() && TFGameRules()->GetGameType() != TF_GAMETYPE_CP && TFGameRules()->GetGameType() != TF_GAMETYPE_ARENA )
			return;

		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			 FStrEq( pszEventName, "teamplay_round_active") )
		{
			m_iPointsCapped = 0;
		}
		else if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			int iTeam = event->GetInt( "team" );
			if ( iTeam == pLocalPlayer->GetTeamNumber() )
			{
				const char *cappers = event->GetString( "cappers" );
				for ( int i=0; i<Q_strlen( cappers ); i++ )
				{
					int iPlayerIndex = (int) cappers[i];
					CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
					if ( pPlayer == pLocalPlayer )
					{
						m_iPointsCapped++;
					}
				}
			}

			if ( m_iPointsCapped == 3 )
			{
				IncrementCount();
			}
		}
	}

	int m_iPointsCapped;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_CaptureThreePoints, ACHIEVEMENT_TF_SCOUT_CAPTURE_THREE_POINTS, "TF_SCOUT_CAPTURE_THREE_POINTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_FastCap : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_startcapture" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// only on maps with capture points
		if ( TFGameRules() && TFGameRules()->GetGameType() != TF_GAMETYPE_CP && TFGameRules()->GetGameType() != TF_GAMETYPE_ARENA )
			return;

		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( pszEventName, "teamplay_point_startcapture" ) )
		{
			const char *cappers = event->GetString( "cappers" );
			for ( int i=0; i<Q_strlen( cappers ); i++ )
			{
				int iPlayerIndex = (int) cappers[i];
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
				if ( pPlayer == pLocalPlayer )
				{
					if ( event->GetFloat( "captime" ) < 1.0 )
					{
						IncrementCount();
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_FastCap, ACHIEVEMENT_TF_SCOUT_FAST_CAP, "TF_SCOUT_FAST_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_StartAndFinishCap : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );

		m_Points.Purge();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_capture_broken" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		// Achievement for initiating captures that ultimately succeed.
		if ( FStrEq( pszEventName, "teamplay_point_startcapture" ) )
		{
			const char *cappers = event->GetString( "cappers" );
			for ( int i=0; i<Q_strlen( cappers ); i++ )
			{
				int iPlayerIndex = (int) cappers[i];
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
				if ( pPlayer == pLocalPlayer ) // Are we one of the cappers?
				{
					// We started capping this point.
					m_Points.AddToTail( event->GetInt( "cp" ) );
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_point_captured" ) )
		{
			int iIndex = m_Points.Find( event->GetInt( "cp" ) );
			if ( iIndex > -1 ) // Is the point one we started capping?
			{
				int iTeam = event->GetInt( "team" );
				if ( iTeam == pLocalPlayer->GetTeamNumber() )
				{
					m_Points.Remove( iIndex );
					IncrementCount();					
				}
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_capture_broken" ) )
		{
			// Our team failed to complete the capture.
			int iIndex = m_Points.Find( event->GetInt( "cp" ) );
			if ( iIndex > -1 )
			{
				m_Points.Remove( iIndex );
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			// Reset tracking at the start of a new round.
			m_Points.Purge();
		}
	}

	CUtlVector< int >	m_Points;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_StartAndFinishCap, ACHIEVEMENT_TF_SCOUT_START_AND_FINISH_CAP, "TF_SCOUT_START_AND_FINISH_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_BlockCaps : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_capture_blocked" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		// Achievement for blocking captures!
		if ( FStrEq( pszEventName, "teamplay_capture_blocked" ) )
		{
			int iBlocker = event->GetInt( "blocker" );
			if ( (iBlocker == pLocalPlayer->entindex()) &&
				 ((TFGameRules()->GetGameType() == TF_GAMETYPE_CP) ||
				  (TFGameRules()->GetGameType() == TF_GAMETYPE_ARENA)) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_BlockCaps, ACHIEVEMENT_TF_SCOUT_BLOCK_CAPS, "TF_SCOUT_BLOCK_CAPS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_CarrierKillCarrier : public CBaseTFAchievement
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
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		// Achievement for blocking captures!
		if ( FStrEq( pszEventName, "teamplay_flag_event" ) )
		{
			int iType = event->GetInt( "eventtype" );
			if ( iType == TF_FLAGEVENT_DEFEND )
			{
				int iPlayer = event->GetInt( "player" );
				if ( (iPlayer == pLocalPlayer->entindex()) && pLocalPlayer->HasTheFlag() )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_CarrierKillCarrier, ACHIEVEMENT_TF_SCOUT_CARRIER_KILL_CARRIER, "TF_SCOUT_CARRIER_KILL_CARRIER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_CapFlagWithoutAttacking : public CBaseTFAchievement
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
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		// Achievement for capping the flag without shooting anyone.
		if ( FStrEq( pszEventName, "teamplay_flag_event" ) )
		{
			int iType = event->GetInt( "eventtype" );
			if ( iType == TF_FLAGEVENT_CAPTURE )
			{
				int iPlayer = event->GetInt( "player" );
				if ( (iPlayer == pLocalPlayer->entindex()) && !pLocalPlayer->HasFiredWeapon() )
				{
					IncrementCount();
				}
			}
			else if ( iType == TF_FLAGEVENT_PICKUP )
			{
				int iPlayer = event->GetInt( "player" );
				if ( iPlayer == pLocalPlayer->entindex() )
				{
					pLocalPlayer->SetFiredWeapon( false );
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_CapFlagWithoutAttacking, ACHIEVEMENT_TF_SCOUT_CAP_FLAG_WITHOUT_ATTACKING, "TF_SCOUT_CAP_FLAG_WITHOUT_ATTACKING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_LongDistanceRunner : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25000 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_death" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		// Achievement for running ...
		bool bUpdate = false;
		if ( FStrEq( pszEventName, "player_death" ) )
		{
			int iVictimID = event->GetInt( "userid" );
			if ( pLocalPlayer && (iVictimID == pLocalPlayer->GetUserID()) )
			{
				bUpdate = true;
			}
		}
		else if ( FStrEq( pszEventName, "teamplay_round_win" ) )
		{
			bUpdate = true;
		}

		if ( bUpdate )
		{
			float fMeters = pLocalPlayer->GetMetersRan();
			IncrementCount( ceil( fMeters ) );
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_LongDistanceRunner, ACHIEVEMENT_TF_SCOUT_LONG_DISTANCE_RUNNER, "TF_SCOUT_LONG_DISTANCE_RUNNER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_TauntKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_iTargetID = 0;
		m_vSlamOrigin = Vector(0,0,0);
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "scout_grand_slam" );
		ListenForGameEvent( "scout_slamdoll_landed" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		const char *pszEventName = event->GetName();
		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			// If we died or respawned, reset...
			m_iTargetID = 0;
			m_vSlamOrigin = Vector(0,0,0);
			return;
		}
		else if ( FStrEq( pszEventName, "scout_grand_slam" ) )
		{
			int iScoutID = event->GetInt( "scout_id" );
			if ( pLocalPlayer && (iScoutID == pLocalPlayer->GetUserID()) )
			{
				m_iTargetID = event->GetInt( "target_id" ); // target_id is a userid
				int iTarget = engine->GetPlayerForUserID( m_iTargetID );
				CBaseEntity *pTarget = UTIL_PlayerByIndex( iTarget );
				if ( pTarget )
				{
					m_vSlamOrigin = pTarget->GetAbsOrigin();
				}
			}
		}
		else if ( FStrEq( pszEventName, "scout_slamdoll_landed" ) )
		{
			// Josh: This comes straight through from a GetRefEHandle().ToInt()
			// so this is safe.
			CBaseHandle hPlayer = CBaseHandle::UnsafeFromIndex( event->GetInt( "target_index" ) );
			C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer* >( hPlayer.Get() );
			if ( pPlayer && (m_iTargetID == pPlayer->GetUserID()) &&
				 (m_vSlamOrigin != Vector(0,0,0)) )
			{
				float x = event->GetFloat( "x" );
				float y = event->GetFloat( "y" );
				float z = event->GetFloat( "z" );
				Vector vHitOrigin = Vector(x,y,z);
				float fDist = (vHitOrigin - m_vSlamOrigin).Length();
				if ( fDist > 1000 )
				{
					IncrementCount();
				}
			}
		}
	}

	int	m_iTargetID;
	Vector m_vSlamOrigin;
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_TauntKill, ACHIEVEMENT_TF_SCOUT_TAUNT_KILL, "TF_SCOUT_TAUNT_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFScout_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 10, ACHIEVEMENT_TF_SCOUT_START_RANGE, ACHIEVEMENT_TF_SCOUT_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_AchieveProgress1, ACHIEVEMENT_TF_SCOUT_ACHIEVE_PROGRESS1, "TF_SCOUT_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFScout_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 16, ACHIEVEMENT_TF_SCOUT_START_RANGE, ACHIEVEMENT_TF_SCOUT_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_AchieveProgress2, ACHIEVEMENT_TF_SCOUT_ACHIEVE_PROGRESS2, "TF_SCOUT_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFScout_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 22, ACHIEVEMENT_TF_SCOUT_START_RANGE, ACHIEVEMENT_TF_SCOUT_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_AchieveProgress3, ACHIEVEMENT_TF_SCOUT_ACHIEVE_PROGRESS3, "TF_SCOUT_ACHIEVE_PROGRESS3", 5 );

// Receive the DamageDodged user message and send out a clientside event for achievements to hook.
USER_MESSAGE( DamageDodged )
{
	int iDamage = msg.ReadShort();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_damage_dodged" );
	if ( event )
	{
		event->SetInt( "damage", iDamage );
		gameeventmanager->FireEventClientSide( event );
	}
}

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_BackscatterKillSpyGrind : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) )
			{
				if ( ( event->GetInt( "weaponid" ) == TF_WEAPON_SCATTERGUN ) && ( event->GetInt( "damagebits" ) & DMG_CRITICAL ) )
				{
					CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
					if ( pTFAttacker )
					{
						CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
						if ( pWeapon )
						{
							int iMiniCritBackAttack = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritBackAttack, closerange_backattack_minicrits );
							if ( iMiniCritBackAttack > 0 )
							{
								IncrementCount();
							}
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_BackscatterKillSpyGrind, ACHIEVEMENT_TF_SCOUT_BACKSCATTER_KILL_SPY_GRIND, "TF_SCOUT_BACKSCATTER_KILL_SPY_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_BackscatterKillHeavyMedicPair : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
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
		m_hTargets[iTargetIndex].flTimeToBeat = gpGlobals->curtime + 20.0f; // 20 seconds to kill the target
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_SCATTERGUN )
			{
				CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				if ( pTFAttacker )
				{
					CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
					if ( pWeapon )
					{
						int iMiniCritBackAttack = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritBackAttack, closerange_backattack_minicrits );
						if ( iMiniCritBackAttack > 0 )
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
								CTFPlayer *pNewTarget = NULL;
								CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
								if ( pTFVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
								{
									for ( int i = 1; i <= gpGlobals->maxClients; i++ )
									{
										pNewTarget = ToTFPlayer( UTIL_PlayerByIndex( i ) );
										if ( pNewTarget && pNewTarget->IsPlayerClass( TF_CLASS_MEDIC ) && ( pNewTarget->MedicGetHealTarget() == pTFVictim ) )
										{
											// add all of his Medics to our list of targets (could be more than one Medic)
											AddNewTarget( pNewTarget );
										}
									}
								}
								else if ( pTFVictim->IsPlayerClass( TF_CLASS_MEDIC ) )
								{
									pNewTarget = ToTFPlayer( pTFVictim->MedicGetHealTarget() );
									if ( pNewTarget && ( pNewTarget->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) ) )
									{
										AddNewTarget( pNewTarget );
									}
								}
							}
						}
					}
				}
			}
		}

		// is this victim in our list of targets?
		int index = GetTargetIndex( pVictim );
		if ( index != -1 )
		{
			m_hTargets.Remove( index );
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
DECLARE_ACHIEVEMENT( CAchievementTFScout_BackscatterKillHeavyMedicPair, ACHIEVEMENT_TF_SCOUT_BACKSCATTER_KILL_HEAVY_MEDIC_PAIR, "TF_SCOUT_BACKSCATTER_KILL_HEAVY_MEDIC_PAIR", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFScout_BackscatterKillFriendsGrind : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			if ( ( event->GetInt( "weaponid" ) == TF_WEAPON_SCATTERGUN ) && ( event->GetInt( "damagebits" ) & DMG_CRITICAL ) )
			{
				CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				if ( pTFAttacker )
				{
					CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
					if ( pWeapon )
					{
						int iMiniCritBackAttack = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritBackAttack, closerange_backattack_minicrits );
						if ( iMiniCritBackAttack > 0 )
						{
							if ( !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils() || !g_pGameRules->IsMultiplayer() )
								return;

							player_info_t pi;
							if ( !engine->GetPlayerInfo( pVictim->entindex(), &pi ) )
								return;

							if ( !pi.friendsID )
								return;

							// check and see if they're on the local player's friends list
							CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
							if ( steamapicontext->SteamFriends()->HasFriend( steamID, k_EFriendFlagImmediate ) )
							{
								IncrementCount();
							}
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFScout_BackscatterKillFriendsGrind, ACHIEVEMENT_TF_SCOUT_BACKSCATTER_KILL_FRIENDS_GRIND, "TF_SCOUT_BACKSCATTER_KILL_FRIENDS_GRIND", 5 );

#endif // CLIENT_DLL



