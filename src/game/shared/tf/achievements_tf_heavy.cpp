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

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_DamageTaken : public CBaseTFAchievement
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
			m_iDamageTotal += event->GetInt( "amount" );

			if ( m_iDamageTotal >= 1000 )
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
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_DamageTaken, ACHIEVEMENT_TF_HEAVY_DAMAGE_TAKEN, "TF_HEAVY_DAMAGE_TAKEN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_DamageTypesTaken : public CBaseTFAchievement
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
		m_iDamageTypesTaken = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_damaged" ) == 0 )
		{
			int iDmgType = event->GetInt( "type" );

			// Melee weapons claim to be bullet & club. If dmg type includes club, ignore the bullet.
			if ( iDmgType & (DMG_CLUB|DMG_SLASH) )
			{
				iDmgType &= ~DMG_BULLET;
			}
			m_iDamageTypesTaken |= iDmgType;

			// Get the achievement once we've been shot, burned, clubbed, and exploded.
			if ( (m_iDamageTypesTaken & (DMG_BULLET|DMG_BUCKSHOT)) && (m_iDamageTypesTaken & DMG_BLAST) && 
				 (m_iDamageTypesTaken & (DMG_BURN|DMG_IGNITE)) && (m_iDamageTypesTaken & (DMG_CLUB|DMG_SLASH)) )
			{
				IncrementCount();
			}
		}
		else if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_iDamageTypesTaken = 0;
		}
		else if ( FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iDamageTypesTaken = 0;
		}
	}

private:
	int		m_iDamageTypesTaken;
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_DamageTypesTaken, ACHIEVEMENT_TF_HEAVY_TAKE_MULTI_DAMAGE, "TF_HEAVY_TAKE_MULTI_DAMAGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_SurviveCrocket : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// Survive a direct hit from a critical rocket
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_SurviveCrocket, ACHIEVEMENT_TF_HEAVY_SURVIVE_CROCKET, "TF_HEAVY_SURVIVE_CROCKET", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_UncoverSpies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) && pTFVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			bool bSuccess = false;

			// is the local player the killer or the assister?
			if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
			{
				bSuccess = true;
			}
			else
			{
				// did the local player assist in the kill?
				int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
				if ( iAssisterIndex > 0 )
				{
					if ( iAssisterIndex == GetLocalPlayerIndex() )
					{
						bSuccess = true;
					}
				}
			}
			
			if ( bSuccess )
			{
				IncrementCount(); 
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_UncoverSpies, ACHIEVEMENT_TF_HEAVY_UNCOVER_SPIES, "TF_HEAVY_UNCOVER_SPIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_AssistGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		// did the local player assist in the kill?
		int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		if ( iAssisterIndex > 0 )
		{
			if ( iAssisterIndex == GetLocalPlayerIndex() )
			{
				IncrementCount(); 
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_AssistGrind, ACHIEVEMENT_TF_HEAVY_ASSIST_GRIND, "TF_HEAVY_ASSIST_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillHeaviesGloves : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && event->GetInt( "weaponid" ) == TF_WEAPON_FISTS )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				if ( FStrEq( event->GetString( "weapon_logclassname", "" ), "gloves" ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillHeaviesGloves, ACHIEVEMENT_TF_HEAVY_KILL_HEAVIES_GLOVES, "TF_HEAVY_KILL_HEAVIES_GLOVES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_AssistHeavyGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pLocalPlayer )
		{
			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
			if ( iAssisterIndex > 0 )
			{
				C_TFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				C_TFPlayer *pTFAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
				if ( pTFAssister == pLocalPlayer || pTFAttacker == pLocalPlayer )
				{
					if ( pTFAssister && pTFAssister->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
					{
						IncrementCount(); 
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_AssistHeavyGrind, ACHIEVEMENT_TF_HEAVY_ASSIST_HEAVY_GRIND, "TF_HEAVY_ASSIST_HEAVY_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillDominated : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			bool bDomination = event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION;

			if ( !bDomination ) // we didn't dominate them with *THIS* kill
			{
				CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				
				if ( pTFAttacker && pTFAttacker->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
				{
					IncrementCount(); 
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillDominated, ACHIEVEMENT_TF_HEAVY_KILL_DOMINATED, "TF_HEAVY_KILL_DOMINATED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillCritPunch : public CBaseTFAchievement
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

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && event->GetInt( "weaponid" ) == TF_WEAPON_FISTS )
		{
			if ( event->GetInt( "damagebits" ) & DMG_CRITICAL )
			{
				IncrementCount(); 
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillCritPunch, ACHIEVEMENT_TF_HEAVY_KILL_CRIT_PUNCH, "TF_HEAVY_KILL_CRIT_PUNCH", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillUnderwater : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pAttacker == pLocalPlayer && pVictim != pLocalPlayer )
		{
			if ( pLocalPlayer->GetWaterLevel() >= WL_Eyes && pVictim->GetWaterLevel() >= WL_Eyes )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillUnderwater, ACHIEVEMENT_TF_HEAVY_KILL_UNDERWATER, "TF_HEAVY_KILL_UNDERWATER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_HealMedikits : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_iHealTotal = 0;
	}

	virtual void UpdateAchievement( int nData )
	{
		if ( !LocalPlayerCanEarn() )
			return;

		m_iHealTotal += nData;

		if ( m_iHealTotal >= 1000 )
		{
			IncrementCount();
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iHealTotal = 0;
		}
	}

private:
	int		m_iHealTotal;
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_HealMedikits, ACHIEVEMENT_TF_HEAVY_HEAL_MEDIKITS, "TF_HEAVY_HEAL_MEDIKITS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_ReceiveUberGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// client fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_ReceiveUberGrind, ACHIEVEMENT_TF_HEAVY_RECEIVE_UBER_GRIND, "TF_HEAVY_RECEIVE_UBER_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillWhileSpunup : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "localplayer_winddown" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_nNumKilled = 0;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && event->GetInt( "weaponid" ) == TF_WEAPON_MINIGUN )
		{
			m_nNumKilled++;

			if ( m_nNumKilled >= 5 )
			{
				IncrementCount();
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "localplayer_winddown" ) ||
			 FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_nNumKilled = 0;
		}
	}

private:
	int		m_nNumKilled;
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillWhileSpunup, ACHIEVEMENT_TF_HEAVY_KILL_WHILE_SPUNUP, "TF_HEAVY_KILL_WHILE_SPUNUP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_PayloadCapGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_PayloadCapGrind, ACHIEVEMENT_TF_HEAVY_PAYLOAD_CAP_GRIND, "TF_HEAVY_PAYLOAD_CAP_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillMidAirMinigun : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && event->GetInt( "weaponid" ) == TF_WEAPON_MINIGUN )
		{
			if ( !( pVictim->GetFlags() & FL_ONGROUND ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillMidAirMinigun, ACHIEVEMENT_TF_HEAVY_KILL_MIDAIR_MINIGUN, "TF_HEAVY_KILL_MIDAIR_MINIGUN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_DefendControlPoint : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_DefendControlPoint, ACHIEVEMENT_TF_HEAVY_DEFEND_CONTROL_POINT, "TF_HEAVY_DEFEND_CONTROL_POINT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_FireLots : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// client weapon fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_FireLots, ACHIEVEMENT_TF_HEAVY_FIRE_LOTS, "TF_HEAVY_FIRE_LOTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillWithShotgun : public CBaseTFAchievement
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

		CTFPlayer *pAttackingPlayer = ToTFPlayer( pAttacker );

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && pAttackingPlayer->GetAmmoCount( TF_AMMO_PRIMARY ) <= 0 && 
			event->GetInt( "weaponid" ) == TF_WEAPON_SHOTGUN_HWG )
		{
				IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillWithShotgun, ACHIEVEMENT_TF_HEAVY_KILL_SHOTGUN, "TF_HEAVY_KILL_SHOTGUN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_EarnDominationForMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_EarnDominationForMedic, ACHIEVEMENT_TF_HEAVY_EARN_MEDIC_DOMINATION, "TF_HEAVY_EARN_MEDIC_DOMINATION", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_ClearStickybombs : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_ClearStickybombs, ACHIEVEMENT_TF_HEAVY_CLEAR_STICKYBOMBS, "TF_HEAVY_CLEAR_STICKYBOMBS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_FirstToCap : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		m_bTeamCappedThisRound = false;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_bTeamCappedThisRound = false;
		}
		else if ( FStrEq( event->GetName(), "teamplay_point_startcapture" ) )
		{
			// only on maps with capture points
			if ( TFGameRules() && TFGameRules()->GetGameType() != TF_GAMETYPE_CP && TFGameRules()->GetGameType() != TF_GAMETYPE_ARENA )
				return;

			// we've already started a capture this round
			if ( m_bTeamCappedThisRound )
				return;

			// is this event about our team?
			if ( GetLocalPlayerTeam() == event->GetInt( "capteam" ) )
			{
				m_bTeamCappedThisRound = true;

				// we need to have started the capture by ourselves...not standing on the point with a group of people
				const char *cappers = event->GetString( "cappers" );
				if ( Q_strlen( cappers ) == 1 )
				{
					// is the capper the local player?
					if ( GetLocalPlayerIndex() == (int)cappers[0] )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	bool	m_bTeamCappedThisRound;
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_FirstToCap, ACHIEVEMENT_TF_HEAVY_FIRST_TO_CAP, "TF_HEAVY_FIRST_TO_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillTaunt : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillTaunt, ACHIEVEMENT_TF_HEAVY_KILL_TAUNT, "TF_HEAVY_KILL_TAUNT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_TeleportFastKill : public CTFAchievementTeleporterTimingKills<CBaseTFAchievement>
{
	// stub -- all code in parent class
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_TeleportFastKill, ACHIEVEMENT_TF_HEAVY_TELEPORT_FAST_KILL, "TF_HEAVY_TELEPORT_FAST_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_FreezecamTaunt : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_FreezecamTaunt, ACHIEVEMENT_TF_HEAVY_FREEZECAM_TAUNT, "TF_HEAVY_FREEZECAM_TAUNT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_RevengeAssist : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			// did the assister get revenge?
			if ( event->GetInt( "death_flags" ) & TF_DEATH_ASSISTER_REVENGE )
			{
				IncrementCount();
			}
		}
		else
		{
			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
			if ( iAssisterIndex > 0 )
			{
				if ( iAssisterIndex == GetLocalPlayerIndex() )
				{
					// did the attacker get revenge?
					if ( event->GetInt( "death_flags" ) & TF_DEATH_REVENGE  )
					{
						IncrementCount();
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_RevengeAssist, ACHIEVEMENT_TF_HEAVY_REVENGE_ASSIST, "TF_HEAVY_REVENGE_ASSIST", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_DefendMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_DefendMedic, ACHIEVEMENT_TF_HEAVY_DEFEND_MEDIC, "TF_HEAVY_DEFEND_MEDIC", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillCappingEnemies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 15 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillCappingEnemies, ACHIEVEMENT_TF_HEAVY_KILL_CAPPING_ENEMIES, "TF_HEAVY_KILL_CAPPING_ENEMIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_BlockInvulnHeavy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_BlockInvulnHeavy, ACHIEVEMENT_TF_HEAVY_BLOCK_INVULN_HEAVY, "TF_HEAVY_BLOCK_INVULN_HEAVY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillFlagCarriers : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
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
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillFlagCarriers, ACHIEVEMENT_TF_HEAVY_KILL_FLAG_CARRIERS, "TF_HEAVY_KILL_FLAG_CARRIERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_StandNearDispenser : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_StandNearDispenser, ACHIEVEMENT_TF_HEAVY_STAND_NEAR_DISPENSER, "TF_HEAVY_STAND_NEAR_DISPENSER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_EatSandwiches : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_EatSandwiches, ACHIEVEMENT_TF_HEAVY_EAT_SANDWICHES, "TF_HEAVY_EAT_SANDWICHES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillScouts : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && event->GetInt( "weaponid" ) == TF_WEAPON_MINIGUN )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SCOUT ) )
			{
				if ( FStrEq( event->GetString( "weapon_logclassname", "" ), "natascha" ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillScouts, ACHIEVEMENT_TF_HEAVY_KILL_SCOUTS, "TF_HEAVY_KILL_SCOUTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_BlockCart : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_capture_blocked" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !TFGameRules() || TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT )
			return;

		if ( Q_strcmp( event->GetName(), "teamplay_capture_blocked" ) == 0 )
		{
			int index = event->GetInt( "blocker", 0 );
			if ( index == GetLocalPlayerIndex() )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_BlockCart, ACHIEVEMENT_TF_HEAVY_BLOCK_CART, "TF_HEAVY_BLOCK_CART", 5 );

//----------------------------------------------------------------------------------------------------------------
#define MAX_PARTNERS 12
class CAchievementTFHeavy_AssistMedicLarge : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_Partners.Purge();
		}
	}

	int	GetPartnerIndex( CBaseEntity *pPlayer )
	{
		for ( int i = 0; i < m_Partners.Count(); i++ )
		{
			if ( m_Partners[i].hPartner == pPlayer )
				return i;
		}
		return -1;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			if ( pVictim->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
			{
				if ( pLocalPlayer == pAttacker )
				{
					int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
					if ( iAssisterIndex > 0 )
					{
						C_TFPlayer *pTFAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
						if ( pTFAssister && pTFAssister->IsPlayerClass( TF_CLASS_MEDIC ) )
						{
							int index = GetPartnerIndex( pTFAssister );
							if ( index == -1 )
							{
								if ( m_Partners.Count() >= MAX_PARTNERS )
								{
									// Remove the one with the least assists
									int iLowest = 999;
									int iLowestIndex = -1;
									for ( int i = 0; i < m_Partners.Count(); i++ )
									{
										if ( !m_Partners[i].hPartner )
										{
											// Player is gone. Lets remove that one.
											iLowestIndex = i;
											break;
										}

										if ( m_Partners[i].iAssists < iLowest )
										{
											iLowestIndex = i;
											iLowest = m_Partners[i].iAssists;
										}
									}

									if ( iLowestIndex >= 0 )
									{
										//Msg("FULL Removed %d (%s)\n", iLowestIndex, g_PR->GetPlayerName(m_Partners[iLowestIndex].hPartner->entindex()) );
										m_Partners.Remove(iLowestIndex);
									}
								}

								int iNewIndex = m_Partners.AddToTail();
								m_Partners[iNewIndex].hPartner = pTFAssister;
								m_Partners[iNewIndex].iAssists = 1;

								//Msg("Inserted %s into %d\n", g_PR->GetPlayerName(pTFAssister->entindex()), iNewIndex );
							}
							else
							{
								m_Partners[index].iAssists++;

								//Msg("Incremented %s in %d to %d\n", g_PR->GetPlayerName(m_Partners[index].hPartner->entindex()), index, m_Partners[index].iAssists );

								if ( m_Partners[index].iAssists >= 10 )
								{
									IncrementCount();
								}
							}
						}
						else
						{
							// Ensure this guy isn't in our list. We can have non-medics in our list if we
							// earn an assist with them, and then they switch classes in the respawn room.
							int index = GetPartnerIndex( pTFAssister );
							if ( index != -1 )
							{
								m_Partners.Remove(index);
							}
						}
					}
				}
			}

			// See if it's one of our partners
			int index = GetPartnerIndex( pVictim );
			if ( index != -1 )
			{
				//Msg("DEATH: Removed %d (%s)\n", index, g_PR->GetPlayerName(m_Partners[index].hPartner->entindex()) );
				m_Partners.Remove( index );
			}

			/*
			Msg("State:\n");
			for ( int i = 0; i < m_Partners.Count(); i++ )
			{
				if ( m_Partners[i].hPartner )
				{
					Msg("   %d: %s with %d\n", i, g_PR->GetPlayerName(m_Partners[i].hPartner->entindex()), m_Partners[i].iAssists );
				}
				else
				{
					Msg("   %d: EMPTY\n", i );
				}
			}
			*/
		}
	}

private:
	struct partners_t
	{
		EHANDLE hPartner;
		int		iAssists;
	};

	CUtlVector<partners_t>	m_Partners;
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_AssistMedicLarge, ACHIEVEMENT_TF_HEAVY_ASSIST_MEDIC_LARGE, "TF_HEAVY_ASSIST_MEDIC_LARGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_KillMedicPair : public CBaseTFAchievement
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

	void AddNewTarget( CBaseEntity *pTarget, CBaseEntity *pPartner )
	{
		if ( !pTarget || !pPartner )
			return;

		// see if the target is already in our list or get a new index
		int iTargetIndex = GetTargetIndex( pTarget );
		if ( iTargetIndex == -1 )
		{
			iTargetIndex = m_hTargets.AddToTail();
		}

		m_hTargets[iTargetIndex].hPartner = pPartner;
		m_hTargets[iTargetIndex].hTarget = pTarget;
		m_hTargets[iTargetIndex].flTimeToBeat = gpGlobals->curtime + 15.0f; // 15 seconds to kill the target
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			int iAssisterIndex = engine->GetPlayerForUserID( event->GetInt( "assister" ) );

			if ( iAssisterIndex > 0 )
			{
				C_TFPlayer *pTFAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterIndex ) );
				C_TFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

				if ( ( pTFAttacker == pLocalPlayer && ( pTFAssister && pTFAssister->IsPlayerClass( TF_CLASS_MEDIC ) ) ) ||
					 ( pTFAssister == pLocalPlayer && ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_MEDIC ) ) ) )
				{
					C_TFPlayer *pPartner = ( pTFAttacker == pLocalPlayer ) ? pTFAssister : pTFAttacker;

					// is this victim in our list of targets?
					int index = GetTargetIndex( pVictim );
					if ( index != -1 )
					{
						// did we kill them with the correct partner?
						if ( m_hTargets[index].hPartner == pPartner )
						{
							// did we beat the time?
							if ( m_hTargets[index].flTimeToBeat > gpGlobals->curtime )
							{
								IncrementCount();
							}
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
									AddNewTarget( pNewTarget, pPartner );
								}
							}
						}
						else if ( pTFVictim->IsPlayerClass( TF_CLASS_MEDIC ) )
						{
							pNewTarget = ToTFPlayer( pTFVictim->MedicGetHealTarget() );
							if ( pNewTarget && pNewTarget->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
							{	
								AddNewTarget( pNewTarget, pPartner );
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
		EHANDLE hPartner;
		EHANDLE hTarget;
		float flTimeToBeat;
	};

	CUtlVector<targets_t> m_hTargets;
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_KillMedicPair, ACHIEVEMENT_TF_HEAVY_KILL_MEDIC_PAIR, "TF_HEAVY_KILL_MEDIC_PAIR", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFHeavy_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 10, ACHIEVEMENT_TF_HEAVY_START_RANGE, ACHIEVEMENT_TF_HEAVY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_AchieveProgress1, ACHIEVEMENT_TF_HEAVY_ACHIEVE_PROGRESS1, "TF_HEAVY_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFHeavy_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 16, ACHIEVEMENT_TF_HEAVY_START_RANGE, ACHIEVEMENT_TF_HEAVY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_AchieveProgress2, ACHIEVEMENT_TF_HEAVY_ACHIEVE_PROGRESS2, "TF_HEAVY_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHeavy_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFHeavy_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 22, ACHIEVEMENT_TF_HEAVY_START_RANGE, ACHIEVEMENT_TF_HEAVY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHeavy_AchieveProgress3, ACHIEVEMENT_TF_HEAVY_ACHIEVE_PROGRESS3, "TF_HEAVY_ACHIEVE_PROGRESS3", 5 );

#endif // CLIENT_DLL



