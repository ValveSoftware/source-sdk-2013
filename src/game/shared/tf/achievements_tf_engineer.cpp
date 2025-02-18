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
#include "c_obj_sentrygun.h"

//======================================================================================================================================
// ENGINEER ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
// Use a revenge crit to kill the player that destroyed your sentry.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_RevengeCritForSentryKiller : public CBaseTFAchievement
{
protected:
	CBaseEntity* m_pSentryKiller;

public:
	DECLARE_CLASS( CAchievementTFEngineer_RevengeCritForSentryKiller, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		ListenForGameEvent( "object_destroyed" );
		ResetTracking();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "object_destroyed" );
		ResetTracking();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !m_pSentryKiller || !pVictim || !pAttacker )
			return;

		// if the local player has killed the thing that killed it's sentry, 
		// give the achievement
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() &&
			pVictim == m_pSentryKiller &&
			event->GetInt( "customkill" ) == TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT )
		{
			AwardAchievement();
		}
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		// if the object getting killed is the local player's sentry gun,
		// store away who the killer was
		if ( FStrEq( event->GetName(), "object_destroyed" ) )
		{
			int iObject = event->GetInt( "objecttype" );
			if ( iObject == OBJ_SENTRYGUN )
			{
				int iEngineerIdx = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				CBaseEntity *pOwner = UTIL_PlayerByIndex( iEngineerIdx );
				if ( pOwner == pLocalPlayer )
				{
					int iAttackerIdx = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
					CBaseEntity *pKiller = UTIL_PlayerByIndex( iAttackerIdx );
					if ( pKiller != NULL )
					{
						m_pSentryKiller = pKiller;
					}
				}
			}
		}
	}

	void ResetTracking()
	{
		m_pSentryKiller = NULL;
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_RevengeCritForSentryKiller, ACHIEVEMENT_TF_ENGINEER_REVENGE_CRIT_SENTRY_KILLER, "TF_ENGINEER_REVENGE_CRIT_SENTRY_KILLER", 5 );

//----------------------------------------------------------------------------------------------------------------
// Kill X enemies outside the normal sentry gun range using manual control.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_ManualSentryKillsBeyondRange : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_ManualSentryKillsBeyondRange, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_ManualSentryKillsBeyondRange, ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_KILLS_BEYOND_RANGE, "TF_ENGINEER_MANUAL_SENTRY_KILLS_BEYOND_RANGE", 5 );
																			
//----------------------------------------------------------------------------------------------------------------
// Have a manual sentry absorb X amount of damage during the course of its life.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_ManualSentryAbsorbDamage : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_ManualSentryAbsorbDamage, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_ManualSentryAbsorbDamage, ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_ABSORB_DMG, "TF_ENGINEER_MANUAL_SENTRY_ABSORB_DMG", 5 );

//----------------------------------------------------------------------------------------------------------------
// Help a teammate build a structure.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_HelpTeammateBuildStructure : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_HelpTeammateBuildStructure, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_HelpTeammateBuildStructure, ACHIEVEMENT_TF_ENGINEER_HELP_BUILD_STRUCTURE, "TF_ENGINEER_HELP_BUILD_STRUCTURE", 5 );

//----------------------------------------------------------------------------------------------------------------
// Shotgun kill an enemy recently damaged by your sentry gun
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_ShotgunKillPreviousSentryTarget : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_ShotgunKillPreviousSentryTarget, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_ShotgunKillPreviousSentryTarget, ACHIEVEMENT_TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET, "TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET", 5 );

//----------------------------------------------------------------------------------------------------------------
// Have your sentry kill the enemy that just killed you within X seconds.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_SentryAvengesYou : public CBaseTFAchievement
{
protected:
	CBaseEntity* m_pPlayerKiller;
	float m_flPlayerDeathStartTime;

public:
	DECLARE_CLASS( CAchievementTFEngineer_SentryAvengesYou, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		ListenForGameEvent( "object_destroyed" );
		ResetTracking();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "object_destroyed" );
		ResetTracking();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pAttacker || pVictim == pAttacker )
			return;

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		static const float kRevengeTimeLimit = 10.0f;

		// if the local player has been killed (by someone else), keep track of the killer
		if ( pVictim == pLocalPlayer )
		{
			// I was killed by my own sentry while testing, so I figure I meant as well put this check in
			if ( pLocalPlayer->GetTeam() == pAttacker->GetTeam() )
			{
				ResetTracking();
				return;
			}
			m_flPlayerDeathStartTime = gpGlobals->curtime;
			m_pPlayerKiller = pAttacker;
		}
		// if the sentry gun killed the player's killer within 10 seconds of the player's death, award the achievement
		else if ( pVictim == m_pPlayerKiller && m_flPlayerDeathStartTime != 0.0f && (gpGlobals->curtime - m_flPlayerDeathStartTime) < kRevengeTimeLimit )
		{
			int nInflictorEntIndex = event->GetInt( "inflictor_entindex" );
			C_BaseEntity *pRealInflictor = ClientEntityList().GetEnt( nInflictorEntIndex );
			C_ObjectSentrygun *pSentry = dynamic_cast< C_ObjectSentrygun * >( pRealInflictor );
			C_TFProjectile_SentryRocket *pSentryRocket = dynamic_cast< C_TFProjectile_SentryRocket * >( pRealInflictor );
			if ( ( pSentry && pSentry->GetOwner() == pLocalPlayer ) ||
				 ( pSentryRocket && pSentryRocket->GetOwnerEntity() == pLocalPlayer ) )
			{
				ResetTracking();
				AwardAchievement();
			}
		}
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		// if the object getting killed is the local player's sentry gun,
		// reset tracking
		if ( FStrEq( event->GetName(), "object_destroyed" ) )
		{
			int iObject = event->GetInt( "objecttype" );
			if ( iObject == OBJ_SENTRYGUN )
			{
				int iEngineerIdx = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				CBaseEntity *pOwner = UTIL_PlayerByIndex( iEngineerIdx );
				if ( pOwner == pLocalPlayer )
				{
					ResetTracking();
				}
			}
		}
	}

	void ResetTracking()
	{
		m_pPlayerKiller = NULL;
		m_flPlayerDeathStartTime = 0.0f;
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_SentryAvengesYou, ACHIEVEMENT_TF_ENGINEER_SENTRY_AVENGES_YOU, "TF_ENGINEER_SENTRY_AVENGES_YOU", 5 );

//----------------------------------------------------------------------------------------------------------------
// Shotgun kill an enemy recently damaged by your sentry gun
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_RepairRecentlyDamagedSentryWhileBeingHealed : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_RepairRecentlyDamagedSentryWhileBeingHealed, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_RepairRecentlyDamagedSentryWhileBeingHealed, ACHIEVEMENT_TF_ENGINEER_REPAIR_SENTRY_W_MEDIC, "TF_ENGINEER_REPAIR_SENTRY_W_MEDIC", 5 );

//----------------------------------------------------------------------------------------------------------------
// Get X sentry kills on players capturing a point
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_SentryKillPlayersCapturingPoint : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_SentryKillPlayersCapturingPoint, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_SentryKillPlayersCapturingPoint, ACHIEVEMENT_TF_ENGINEER_SENTRY_KILL_CAPS, "TF_ENGINEER_SENTRY_KILL_CAPS", 5 );

//----------------------------------------------------------------------------------------------------------------
// Destroy X sappers on buildings that you do not own
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_DestroySappersOnNonOwnedBuildings : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_DestroySappersOnNonOwnedBuildings, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_DestroySappersOnNonOwnedBuildings, ACHIEVEMENT_TF_ENGINEER_DESTROY_SAPPERS, "TF_ENGINEER_DESTROY_SAPPERS", 5 );

//----------------------------------------------------------------------------------------------------------------
// Reload X rounds done to a sentry under manual control by another Engineer. (could be health / metal / something)
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_HelpManualSentry : public CBaseTFAchievement
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_HelpManualSentry, CBaseAchievement );

	virtual void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_HelpManualSentry, ACHIEVEMENT_TF_ENGINEER_HELP_MANUAL_SENTRY, "TF_ENGINEER_HELP_MANUAL_SENTRY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_RevengeCritLife : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );

		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );

		ResetTracking();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			ResetTracking();
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		int customdmg = event->GetInt( "customkill" );
		if ( customdmg == TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT )
		{
			m_iKills++;
			if ( m_iKills >= 3 )
			{
				AwardAchievement();
			}
		}
	}

	void ResetTracking()
	{
		m_iKills = 0;
	}

private:
	int m_iKills;
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_RevengeCritLife, ACHIEVEMENT_TF_ENGINEER_REVENGE_CRIT_LIFE, "TF_ENGINEER_REVENGE_CRIT_LIFE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_TeleportGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_teleported" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_teleported" ) )
		{
			int userid = event->GetInt( "userid" );
			int builderid = event->GetInt( "builderid" );
			float dist = event->GetFloat( "dist" );
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer && pLocalPlayer->GetUserID() == builderid && pLocalPlayer->GetUserID() != userid && dist > 1200 )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_TeleportGrind, ACHIEVEMENT_TF_ENGINEER_TELEPORT_GRIND, "TF_ENGINEER_TELEPORT_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievemeentTFEngineer_DispenserExtinguish : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_extinguished" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_extinguished" ) )
		{
			int victimid = event->GetInt( "victim" );
			int healerid = event->GetInt( "healer" );
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer && pLocalPlayer->entindex() == healerid && pLocalPlayer->entindex() != victimid && pLocalPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievemeentTFEngineer_DispenserExtinguish, ACHIEVEMENT_TF_ENGINEER_DISPENSER_EXTINGUISH, "TF_ENGINEER_DISPENSER_EXTINGUISH", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_ManualCloakedSpyKill : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) && pTFVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			int customdmg = event->GetInt( "customkill" );
			if ( customdmg == TF_DMG_CUSTOM_PLAYER_SENTRY )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_ManualCloakedSpyKill, ACHIEVEMENT_TF_ENGINEER_MANUAL_CLOAKED_SPY_KILL, "TF_ENGINEER_MANUAL_CLOAKED_SPY_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_UpgradeBuildings : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_upgradedobject" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_upgradedobject" ) )
		{
			int userid = event->GetInt( "userid" );
			bool isbuilder = event->GetInt( "isbuilder" );
			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
			if ( pPlayer == C_TFPlayer::GetLocalPlayer() && !isbuilder )
			{
				IncrementCount();
			}
		}
	}	
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_UpgradeBuildings, ACHIEVEMENT_TF_ENGINEER_UPGRADE_BUILDINGS, "TF_ENGINEER_UPGRADE_BUILDINGS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_SentryKillLifetimeGrind : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 5000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( pAttacker != pLocalPlayer )
			return;

		const char* killer_weapon_name = event->GetString( "weapon" );
		if ( ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) ) ||
			( 0 == Q_strncmp( killer_weapon_name, "obj_sentrygun", 13 ) ) ||
			( 0 == Q_strcmp( killer_weapon_name, "wrangler_kill" ) ) ||
			( 0 == Q_strcmp( killer_weapon_name, "obj_minisentry" ) ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_SentryKillLifetimeGrind, ACHIEVEMENT_TF_ENGINEER_SENTRY_KILL_LIFETIME_GRIND, "TF_ENGINEER_SENTRY_KILL_LIFETIME_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_DispenserHealGroup : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_DispenserHealGroup, ACHIEVEMENT_TF_ENGINEER_DISPENSER_HEAL_GROUP, "TF_ENGINEER_DISPENSER_HEAL_GROUP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_WasteMetalGrind : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10000 );
		SetStoreProgressInSteam( true );
	}
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_WasteMetalGrind, ACHIEVEMENT_TF_ENGINEER_WASTE_METAL_GRIND, "TF_ENGINEER_WASTE_METAL_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_KillFlagCarriers : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && pTFVictim != pAttacker && pTFVictim->HasTheFlag() )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_KillFlagCarriers, ACHIEVEMENT_TF_ENGINEER_KILL_FLAG_CARRIERS, "TF_ENGINEER_KILL_FLAG_CARRIERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_KillDisguisedSpy : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) && 
			 pTFVictim->m_Shared.InCond( TF_COND_DISGUISED ) &&
			 event->GetInt( "weaponid" ) == TF_WEAPON_WRENCH )
		{
			AwardAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_KillDisguisedSpy, ACHIEVEMENT_TF_ENGINEER_KILL_DISGUISED_SPY, "TF_ENGINEER_KILL_DISGUISED_SPY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_FreezeTaunt : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Give opponents freezecams of you taunting
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_FreezeTaunt, ACHIEVEMENT_TF_ENGINEER_FREEZECAM_TAUNT, "TF_ENGINEER_FREEZECAM_TAUNT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_FreezeWithSentry : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Give opponents freezecams of you w/ your sentry
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_FreezeWithSentry, ACHIEVEMENT_TF_ENGINEER_FREEZECAM_SENTRY, "TF_ENGINEER_FREEZECAM_SENTRY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_KillSpyTwoSappers : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );

		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "object_destroyed" );

		ResetTracking();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			ResetTracking();
		}
		else if ( FStrEq( event->GetName(), "object_destroyed" ) )
		{
			CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( !pLocalPlayer )
				return;

			if ( event->GetInt( "objecttype" ) == OBJ_ATTACHMENT_SAPPER &&
				event->GetInt( "attacker" ) == pLocalPlayer->GetUserID() )
			{
				SetTime();
				m_iSapperCount++;
				CheckCompletion();
			}
		}
	}

	void ResetTracking()
	{
		m_iDeadSpyID = -1;
		m_flStartTime = -1;
		m_iSapperCount = 0;
	}

	void SetTime()
	{
		if ( gpGlobals->curtime - m_flStartTime > 10 )
		{
			// The player took too long.
			ResetTracking();
		}

		if ( m_flStartTime == -1 )
		{
			// Start the timer over now.
			m_flStartTime = gpGlobals->curtime;
		}
	}

	void CheckCompletion()
	{
		int iSpyIdx = engine->GetPlayerForUserID( m_iDeadSpyID );
		CTFPlayer *pSpy = dynamic_cast<CTFPlayer*>( UTIL_PlayerByIndex( iSpyIdx ) );
		if ( !pSpy )
			return;

		if ( m_iSapperCount < 2 )
			return;

		AwardAchievement();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) )
		{
			SetTime();
			m_iDeadSpyID = pTFVictim->GetUserID();
			CheckCompletion();
		}
	}

	float m_flStartTime;
	int m_iDeadSpyID;
	int m_iSapperCount;
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_KillSpyTwoSappers, ACHIEVEMENT_TF_ENGINEER_KILL_SPY_TWO_SAPPERS, "TF_ENGINEER_KILL_SPY_TWO_SAPPERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_RemoveSapperLongDist : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );

		ListenForGameEvent( "object_destroyed" );
		ListenForGameEvent( "player_sapped_object" );

		ResetTracking();
	}

	void ResetTracking()
	{
		iSapperIndex = -1;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( event->GetName(), "player_sapped_object" ) )
		{
			if ( pLocalPlayer->GetUserID() == event->GetInt( "ownerid" ) )
			{
				m_vecStart = pLocalPlayer->GetAbsOrigin();
				iSapperIndex = event->GetInt( "sapperid" );
			}
		}
		else if ( FStrEq( event->GetName(), "object_destroyed" ) )
		{
			if ( event->GetInt( "objecttype" ) == OBJ_ATTACHMENT_SAPPER &&
				 event->GetInt( "attacker" ) == pLocalPlayer->GetUserID() &&
				 event->GetInt( "index" ) == iSapperIndex )
			{
				float flDist = pLocalPlayer->GetAbsOrigin().DistTo( m_vecStart );
				if ( flDist > 700 )
				{
					AwardAchievement();
				}
			}
		}
	}

	Vector m_vecStart;
	int iSapperIndex;
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_RemoveSapperLongDist, ACHIEVEMENT_TF_ENGINEER_REMOVE_SAPPER_LONG_DIST, "TF_ENGINEER_REMOVE_SAPPER_LONG_DIST", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_TauntKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		int iEvent = event->GetInt( "customkill" );
		if ( ( iEvent == TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH ) || ( iEvent == TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_TauntKill, ACHIEVEMENT_TF_ENGINEER_TAUNT_KILL, "TF_ENGINEER_TAUNT_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_DestroyStickies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_DestroyStickies, ACHIEVEMENT_TF_ENGINEER_DESTROY_STICKIES, "TF_ENGINEER_DESTROY_STICKIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_KillSnipersSentry : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) &&
			event->GetInt( "customkill" ) == TF_DMG_CUSTOM_PLAYER_SENTRY )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_KillSnipersSentry, ACHIEVEMENT_TF_ENGINEER_KILL_SNIPERS_SENTRY, "TF_ENGINEER_KILL_SNIPERS_SENTRY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_KillSentryWithSentry : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );

		ListenForGameEvent( "object_destroyed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( FStrEq( event->GetName(), "object_destroyed" ) )
		{
			const char* weaponstr = event->GetString( "weapon" );
			if ( event->GetInt( "objecttype" ) == OBJ_SENTRYGUN &&
				event->GetInt( "attacker" ) == pLocalPlayer->GetUserID() &&
				FStrEq( weaponstr, "wrangler_kill" ) )
			{
				AwardAchievement();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_KillSentryWithSentry, ACHIEVEMENT_TF_ENGINEER_KILL_SENTRY_WITH_SENTRY, "TF_ENGINEER_KILL_SENTRY_WITH_SENTRY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_MoveSentryGetKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_MoveSentryGetKill, ACHIEVEMENT_TF_ENGINEER_MOVE_SENTRY_GET_KILL, "TF_ENGINEER_MOVE_SENTRY_GET_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_BuildingCarry : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000 );
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

		// Achievement for carrying buildings ...
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
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_BuildingCarry, ACHIEVEMENT_TF_ENGINEER_BUILDING_CARRY, "TF_ENGINEER_BUILDING_CARRY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_KillAssist : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}


	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_death" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_death" ) )
		{
			int iAttackerIndex = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			if ( iAttackerIndex == 0 )
				return;
			CTFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( iAttackerIndex ) );
			if ( !pAttacker )
				return;

			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
			if ( iAssisterIndex == 0 )
				return;
			CTFPlayer *pAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
			if ( !pAssister )
				return;

			// A sentry gun must have been the killing weapon.
			const char* killer_weapon_name = event->GetString( "weapon" );
			if ( ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) ) ||
				( 0 == Q_strncmp( killer_weapon_name, "obj_sentrygun", 13 ) ) ||
				( 0 == Q_strcmp( killer_weapon_name, "wrangler_kill" ) ) ||
				( 0 == Q_strcmp( killer_weapon_name, "obj_minisentry" ) ) )
			{
				if ( pAttacker == C_BasePlayer::GetLocalPlayer() && pAssister->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					// We are the attacker and the assist is from a engineer.
					IncrementCount();
					return;
				}

				if ( pAssister == C_BasePlayer::GetLocalPlayer() && pAttacker->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					// We are the assister and the kill is from a engineer.
					IncrementCount();
					return;
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_KillAssist, ACHIEVEMENT_TF_ENGINEER_KILL_ASSIST, "TF_ENGINEER_KILL_ASSIST", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_DispenserHealGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100000 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_DispenserHealGrind, ACHIEVEMENT_TF_ENGINEER_DISPENSER_HEAL_GRIND, "TF_ENGINEER_DISPENSER_HEAL_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_RepairTeamGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50000 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_RepairTeamGrind, ACHIEVEMENT_TF_ENGINEER_REPAIR_TEAM_GRIND, "TF_ENGINEER_REPAIR_TEAM_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_TankDamage : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_TankDamage, ACHIEVEMENT_TF_ENGINEER_TANK_DAMAGE, "TF_ENGINEER_TANK_DAMAGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_HeavyAssist : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_HeavyAssist, ACHIEVEMENT_TF_ENGINEER_HEAVY_ASSIST, "TF_ENGINEER_HEAVY_ASSIST", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 5, ACHIEVEMENT_TF_ENGINEER_START_RANGE, ACHIEVEMENT_TF_ENGINEER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_AchieveProgress1, ACHIEVEMENT_TF_ENGINEER_ACHIEVE_PROGRESS1, "TF_ENGINEER_ACHIEVE_PROGRESS1", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 11, ACHIEVEMENT_TF_ENGINEER_START_RANGE, ACHIEVEMENT_TF_ENGINEER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_AchieveProgress2, ACHIEVEMENT_TF_ENGINEER_ACHIEVE_PROGRESS2, "TF_ENGINEER_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFEngineer_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFEngineer_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 17, ACHIEVEMENT_TF_ENGINEER_START_RANGE, ACHIEVEMENT_TF_ENGINEER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFEngineer_AchieveProgress3, ACHIEVEMENT_TF_ENGINEER_ACHIEVE_PROGRESS3, "TF_ENGINEER_ACHIEVE_PROGRESS3", 5 );

#endif // CLIENT_DLL
