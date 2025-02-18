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
#include "tf_weapon_shovel.h"
#include "tf_weapon_rocketlauncher.h"
#include "movevars_shared.h"

//======================================================================================================================================
// SOLDIER ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_RJEqualizerKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_RJEqualizerKill, ACHIEVEMENT_TF_SOLDIER_RJ_EQUALIZER_KILL, "TF_SOLDIER_RJ_EQUALIZER_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_BuffTeammates : public CBaseTFAchievement
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
		ListenForGameEvent( "deploy_buff_banner" );
		ListenForGameEvent( "player_buff" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "teamplay_round_active" ) ||
			 FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			m_nBuffCount = 0;
			m_BuffedPlayers.RemoveAll();
		}
		else if ( FStrEq( pszEvent, "deploy_buff_banner" ) )
		{
			int iSoldier = engine->GetPlayerForUserID( event->GetInt( "buff_owner" ) );
			CBasePlayer *pSoldier = UTIL_PlayerByIndex( iSoldier );
			if ( pSoldier && pSoldier == C_TFPlayer::GetLocalTFPlayer() )
			{
				// the local player has deployed their banner, clear our list for the current banner so we can track who we buff
				m_BuffedPlayers.RemoveAll();
			}
		}
		else if ( FStrEq( pszEvent, "player_buff" ) )
		{
			int iSoldier = engine->GetPlayerForUserID( event->GetInt( "buff_owner" ) );
			CBasePlayer *pSoldier = UTIL_PlayerByIndex( iSoldier );
			if ( pSoldier && pSoldier == C_TFPlayer::GetLocalTFPlayer() )
			{
				int iPlayer = event->GetInt( "userid" );
				if ( iPlayer != pSoldier->GetUserID() )
				{
					// this is not the local player being buffed (can't buff yourself for this achievement)
					int iIndex = m_BuffedPlayers.Find( iPlayer );
					if ( iIndex == m_BuffedPlayers.InvalidIndex() )
					{
						// they're not in our list for the currently deployed banner
						m_BuffedPlayers.AddToTail( iPlayer );
						m_nBuffCount++;

						if ( m_nBuffCount >= 15 )
						{
							IncrementCount();
						}
					}
				}
			}
		}
	}

private:
	int m_nBuffCount;
	CUtlVector< int > m_BuffedPlayers;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_BuffTeammates, ACHIEVEMENT_TF_SOLDIER_BUFF_TEAMMATES, "TF_SOLDIER_BUFF_TEAMMATES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillDemomanGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 500 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			// no friendly fire kills
			if ( pVictim->GetTeamNumber() != pAttacker->GetTeamNumber() )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_DEMOMAN ) )
				{
					// we killed a demoman
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillDemomanGrind, ACHIEVEMENT_TF_SOLDIER_KILL_DEMOMAN_GRIND, "TF_SOLDIER_KILL_DEMOMAN_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillEngy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillEngy, ACHIEVEMENT_TF_SOLDIER_KILL_ENGY, "TF_SOLDIER_KILL_ENGY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillPyro : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		SetDefLessFunc( m_Pyros );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "object_deflected" );
		m_Pyros.RemoveAll();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "teamplay_round_active" ) ||
			 FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			m_Pyros.RemoveAll();
		}
		else if ( FStrEq( pszEvent, "object_deflected" ) )
		{
			int iSoldier = engine->GetPlayerForUserID( event->GetInt( "ownerid" ) );
			CBasePlayer *pSoldier = UTIL_PlayerByIndex( iSoldier );
			if ( pSoldier && pSoldier == C_TFPlayer::GetLocalTFPlayer() )
			{
				int iWeaponID = event->GetInt( "weaponid" );
				bool bRocketLauncherUsed = (iWeaponID == TF_WEAPON_ROCKETLAUNCHER) || (iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT);
				// this was one of our objects...was it a rocket?
				if ( bRocketLauncherUsed )
				{
					// add or update them in our list
					int iIndex = m_Pyros.Find( event->GetInt( "userid" ) );
					if ( iIndex != m_Pyros.InvalidIndex() )
					{
						m_Pyros[iIndex] = gpGlobals->curtime;
					}
					else
					{
						m_Pyros.Insert( event->GetInt( "userid" ), gpGlobals->curtime );
					}
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_PYRO ) )
			{
				int iIndex = m_Pyros.Find( pTFVictim->GetUserID() );
				if ( iIndex != m_Pyros.InvalidIndex() )
				{
					if ( gpGlobals->curtime - m_Pyros[iIndex] <= 10.0f )
					{
						// we killed someone in our list within the required time
						IncrementCount();
					}
				}
			}
		}
	}

private:
	CUtlMap< int, float > m_Pyros; // userID and most recent time they deflected one of our rockets
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillPyro, ACHIEVEMENT_TF_SOLDIER_KILL_PYRO, "TF_SOLDIER_KILL_PYRO", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_NemesisShovelKill : public CBaseTFAchievement
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

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_SHOVEL && event->GetInt( "customkill" ) == 0 )
			{
				// did the assister get revenge?
				if ( event->GetInt( "death_flags" ) & TF_DEATH_REVENGE )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_NemesisShovelKill, ACHIEVEMENT_TF_SOLDIER_NEMESIS_SHOVEL_KILL, "TF_SOLDIER_NEMESIS_SHOVEL_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_DestroyStickies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_nPipesDestroyed = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "player_destroyed_pipebomb" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "teamplay_round_active" ) ||
			 FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			m_nPipesDestroyed = 0;
		}
		else if ( FStrEq( pszEvent, "player_destroyed_pipebomb" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( pLocalTFPlayer && pLocalTFPlayer->GetUserID() == iUserID )
			{
				if ( pLocalTFPlayer->GetActiveTFWeapon() )
				{
					if ( pLocalTFPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_SHOTGUN_SOLDIER )
					{
						m_nPipesDestroyed++;

						if ( m_nPipesDestroyed >= 10 )
						{
							IncrementCount();
						}
					}
				}
			}
		}
	}

private:
	int m_nPipesDestroyed;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_DestroyStickies, ACHIEVEMENT_TF_SOLDIER_DESTROY_STICKIES, "TF_SOLDIER_DESTROY_STICKIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_CrouchRocketJump : public CBaseTFAchievement
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
		ListenForGameEvent( "rocket_jump" );
		ListenForGameEvent( "rocket_jump_landed" );
		ResetTracking();
	}

	void ResetTracking( void )
	{
		ClearThink();
		m_flStartingZ = 0.0f;
	}

	void CheckHeight( void )
	{
		CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( pLocalTFPlayer )
		{
			float flCurrentZ = pLocalTFPlayer->GetAbsOrigin().z;

			// check the settings for gravity
			if ( ( flCurrentZ > m_flStartingZ ) && ( GetCurrentGravity() == 800.0f ) )
			{
				if ( flCurrentZ - m_flStartingZ > 450.0f )
				{
					IncrementCount();
				}
			}
			else
			{
				// we've fallen below our starting Z or someone has messed with gravity, so reset our tracking
				ResetTracking();
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "teamplay_round_active" ) ||
			 FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			ResetTracking();
		}
		else if ( FStrEq( pszEvent, "rocket_jump" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( pLocalTFPlayer && pLocalTFPlayer->GetUserID() == iUserID )
			{
				m_flStartingZ = pLocalTFPlayer->GetAbsOrigin().z;
				SetNextThink( 0.1 );
			}
		}
		else if ( FStrEq( pszEvent, "rocket_jump_landed" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( pLocalTFPlayer && pLocalTFPlayer->GetUserID() == iUserID )
			{
				CheckHeight();
				ResetTracking();
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pVictim == C_BasePlayer::GetLocalPlayer() )
		{
			ResetTracking();
		}
	}

	virtual void Think( void )
	{
		CheckHeight();
		SetNextThink( 0.1 );
	}

private:
	float m_flStartingZ;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_CrouchRocketJump, ACHIEVEMENT_TF_SOLDIER_CROUCH_ROCKET_JUMP, "TF_SOLDIER_CROUCH_ROCKET_JUMP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_EqualizerStreak : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		m_nStreak = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "localplayer_healed" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_SHOVEL )
			{
				CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
				if ( pLocalTFPlayer && pLocalTFPlayer->GetActiveTFWeapon() )
				{
					CTFShovel *pShovel = static_cast< CTFShovel* >( pLocalTFPlayer->GetActiveTFWeapon() );
					if ( pShovel && pShovel->GetShovelType() == SHOVEL_DAMAGE_BOOST )
					{
						m_nStreak++;

						if ( m_nStreak >= 3 )
						{
							IncrementCount();
						}
					}
				}
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) ||
			 FStrEq( event->GetName(), "localplayer_healed" ) )
		{
			m_nStreak = 0;
		}
	}

private:
	int m_nStreak;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_EqualizerStreak, ACHIEVEMENT_TF_SOLDIER_EQUALIZER_STREAK, "TF_SOLDIER_EQUALIZER_STREAK", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_BuffFriends : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// client CTFPlayerShared::PulseSoldierBuff() awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_BuffFriends, ACHIEVEMENT_TF_SOLDIER_BUFF_FRIENDS, "TF_SOLDIER_BUFF_FRIENDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillGroupWithCrocket : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_iKilledBySameRocket = 0;
		m_fDamageTime = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) || 
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iKilledBySameRocket = 0;
			m_fDamageTime = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		int iWeaponID = event->GetInt( "weaponid" );
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && 
			(iWeaponID == TF_WEAPON_ROCKETLAUNCHER || iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT) )
		{
			if ( event->GetInt( "damagebits" ) & DMG_CRITICAL )
			{
				if ( gpGlobals->curtime > m_fDamageTime+1.f )
				{
					m_fDamageTime = gpGlobals->curtime;
					m_iKilledBySameRocket = 0;
				}
				m_iKilledBySameRocket++;
				if ( m_iKilledBySameRocket == 3 )
				{
					IncrementCount(); 
				}
			}
		}
	}

	int			m_iKilledBySameRocket;
	float		m_fDamageTime;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillGroupWithCrocket, ACHIEVEMENT_TF_SOLDIER_KILL_GROUP_WITH_CROCKET, "TF_SOLDIER_KILL_GROUP_WITH_CROCKET", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillTwoDuringRocketJump : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "rocket_jump" );
		ListenForGameEvent( "rocket_jump_landed" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_iKilledDuringRJ = 0;
		m_bRocketJumping = false;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iKilledDuringRJ = 0;
			m_bRocketJumping = false;
		}
		else if ( FStrEq( event->GetName(), "rocket_jump_landed" ) )
		{
			C_TFPlayer *pTFAttacker = NULL;
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				pTFAttacker = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );
				if ( pTFAttacker != C_BasePlayer::GetLocalPlayer() )
					return;
			}
			else
				return;
			m_iKilledDuringRJ = 0;
			m_bRocketJumping = false;
		}
		else if ( FStrEq( event->GetName(), "rocket_jump" ) )
		{
			C_TFPlayer *pTFAttacker = NULL;
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				pTFAttacker = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );
				if ( pTFAttacker != C_BasePlayer::GetLocalPlayer() )
					return;
			}
			else
				return;
			m_iKilledDuringRJ = 0;
			m_bRocketJumping = true;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CTFPlayer *pTFPlayer = ToTFPlayer( pAttacker );
		if ( pTFPlayer && (pTFPlayer == C_BasePlayer::GetLocalPlayer()) )
		{
			if ( m_bRocketJumping )
			{
				m_iKilledDuringRJ++;
				if ( m_iKilledDuringRJ == 2 )
				{
					IncrementCount();
				}
			}
		}
	}

	bool	m_bRocketJumping;
	int		m_iKilledDuringRJ;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillTwoDuringRocketJump, ACHIEVEMENT_TF_SOLDIER_KILL_TWO_DURING_ROCKET_JUMP, "TF_SOLDIER_KILL_TWO_DURING_ROCKET_JUMP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillTaunt : public CBaseTFAchievement
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
			if ( pTFVictim && event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_GRENADE )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillTaunt, ACHIEVEMENT_TF_SOLDIER_KILL_TAUNT, "TF_SOLDIER_KILL_TAUNT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_DefendMedic : public CBaseTFAchievement
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
		ListenForGameEvent( "medic_defended" );
		m_iMedicDefended = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iMedicDefended = 0;
		}
		else if ( FStrEq( event->GetName(), "medic_defended" ) )
		{
			m_iMedicDefended++;
			if ( m_iMedicDefended == 3 )
			{
				IncrementCount();
			}
		}
	}

	int m_iMedicDefended;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_DefendMedic, ACHIEVEMENT_TF_SOLDIER_DEFEND_MEDIC, "TF_SOLDIER_DEFEND_MEDIC", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillWithEqualizerWhileHurt : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		if ( pTFAttacker && (pTFAttacker == C_BasePlayer::GetLocalPlayer()) && (event->GetInt( "weaponid" ) == TF_WEAPON_SHOVEL) )
		{
			CTFShovel *pShovel = static_cast<CTFShovel*>(pTFAttacker->GetActiveWeapon());
			if ( pShovel && (pShovel->GetShovelType() == SHOVEL_DAMAGE_BOOST) )
			{
				if ( pAttacker->GetHealth() < 25 )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillWithEqualizerWhileHurt, ACHIEVEMENT_TF_SOLDIER_KILL_WITH_EQUALIZER_WHILE_HURT, "TF_SOLDIER_KILL_WITH_EQUALIZER_WHILE_HURT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillAirborneTargetWhileAirborne : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && !(pAttacker->GetFlags() & FL_ONGROUND) )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim )
			{
				if ( !(pTFVictim->GetFlags() & FL_ONGROUND) && pTFVictim->IsPlayerClass( TF_CLASS_SOLDIER ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillAirborneTargetWhileAirborne, ACHIEVEMENT_TF_SOLDIER_KILL_AIRBORNE_TARGET_WHILE_AIRBORNE, "TF_SOLDIER_KILL_AIRBORNE_TARGET_WHILE_AIRBORNE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_BounceThenShotgun : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
		SetDefLessFunc( m_BouncedPlayers );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "player_hurt" );
		m_BouncedPlayers.RemoveAll();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			 FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_BouncedPlayers.RemoveAll();
		}
		else if ( FStrEq( event->GetName(), "player_hurt" ) )
		{
			int iSoldier = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			CBasePlayer *pSoldier = UTIL_PlayerByIndex( iSoldier );
			if ( pSoldier && pSoldier == C_TFPlayer::GetLocalTFPlayer() )
			{
				int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				CBasePlayer *pVictim = UTIL_PlayerByIndex( iVictim );
				if ( pVictim )
				{
					//Vector vVictimVelocity = pVictim->GetAbsVelocity();
					bool bVictimGrounded = pVictim->GetFlags() & FL_ONGROUND;
					int iWeaponID = event->GetInt( "weaponid" );
					bool bRocketLauncherUsed = (iWeaponID == TF_WEAPON_ROCKETLAUNCHER) || (iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT);
					if ( bVictimGrounded && bRocketLauncherUsed )
					{
						int iIndex = m_BouncedPlayers.Find( pVictim->GetUserID() );
						if ( iIndex != m_BouncedPlayers.InvalidIndex() )
						{
							// they're already in our list
							m_BouncedPlayers[iIndex] = gpGlobals->curtime;
						}
						else
						{
							// we need to add them
							m_BouncedPlayers.Insert( pVictim->GetUserID(), gpGlobals->curtime );
						}
					}
				}
			}
		}
	}
	
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && !( pTFVictim->GetFlags() & FL_ONGROUND ) && ( event->GetInt( "weaponid" ) == TF_WEAPON_SHOTGUN_SOLDIER ) )
			{
				int iIndex = m_BouncedPlayers.Find( pTFVictim->GetUserID() );
				if ( iIndex != m_BouncedPlayers.InvalidIndex() )
				{
					if ( gpGlobals->curtime < m_BouncedPlayers[iIndex] + 4.f )
					{
						IncrementCount();
					}
				}
			}
		}
	}

	CUtlMap< int, float > m_BouncedPlayers; // userID and most recent time they were bounced
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_BounceThenShotgun, ACHIEVEMENT_TF_SOLDIER_BOUNCE_THEN_SHOTGUN, "TF_SOLDIER_BOUNCE_THEN_SHOTGUN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillAirborneWithDirectHit : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 10 );		
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker != C_BasePlayer::GetLocalPlayer() )
			return;

		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		C_TFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		if ( !pTFAttacker )
			return;

		CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();

		int iMiniCritAirborne = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritAirborne, mini_crit_airborne );
		if ( iMiniCritAirborne == 1 && !(pTFVictim->GetFlags() & FL_ONGROUND) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillAirborneWithDirectHit, ACHIEVEMENT_TF_SOLDIER_KILL_AIRBORNE_WITH_DIRECT_HIT, "TF_SOLDIER_KILL_AIRBORNE_WITH_DIRECT_HIT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillSniperWhileDead : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );		
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		int iWeaponID = event->GetInt( "weaponid" );
		bool bRocketLauncherUsed = (iWeaponID == TF_WEAPON_ROCKETLAUNCHER) || (iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT);
		if ( pTFVictim && (pAttacker == C_TFPlayer::GetLocalTFPlayer()) && (pAttacker->IsAlive() == false) &&
			bRocketLauncherUsed && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillSniperWhileDead, ACHIEVEMENT_TF_SOLDIER_KILL_SNIPER_WHILE_DEAD, "TF_SOLDIER_KILL_SNIPER_WHILE_DEAD", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_DestroySentryOutOfRange : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_DestroySentryOutOfRange, ACHIEVEMENT_TF_SOLDIER_DESTROY_SENTRY_OUT_OF_RANGE, "TF_SOLDIER_DESTROY_SENTRY_OUT_OF_RANGE", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_AssistMedicUbercharge : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
		m_iPlayersGibbed = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_invulned" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_invulned" ) )
		{
			int iTarget = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( iTarget );
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pPlayer == pLocalPlayer )
			{
				m_iPlayersGibbed = 0;
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			if ( event->GetInt( "death_flags" ) & TF_DEATH_GIBBED )
			{
				if ( pLocalPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pLocalPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED ) ||
					pLocalPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
				{
					m_iPlayersGibbed++;
					if ( m_iPlayersGibbed >= 5 )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	int		m_iPlayersGibbed;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_AssistMedicUbercharge, ACHIEVEMENT_TF_SOLDIER_ASSIST_MEDIC_UBER, "TF_SOLDIER_ASSIST_MEDIC_UBER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_ShootMultCrits : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_ShootMultCrits, ACHIEVEMENT_TF_SOLDIER_SHOOT_MULT_CRITS, "TF_SOLDIER_SHOOT_MULT_CRITS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillDefenseless : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual bool LocalPlayerCanEarn( void ) 
	{ 
		if ( TFGameRules() )
		{
			bool bMVMAchievement = ( m_iAchievementID >= ACHIEVEMENT_TF_MVM_START_RANGE && m_iAchievementID <= ACHIEVEMENT_TF_MVM_END_RANGE );

			if ( ( bMVMAchievement && !TFGameRules()->IsMannVsMachineMode() ) || ( !bMVMAchievement && TFGameRules()->IsMannVsMachineMode() ) )
			{
				return false;
			}
		}

		// This achievement can be earned while we're in the post-win state
		return IsLocalTFPlayerClass( TF_CLASS_SOLDIER );
	}

	virtual void ListenForEvents()
	{
		m_iPlayersKilled = 0;
		ListenForGameEvent( "teamplay_win_panel" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_win_panel" ) )
		{
			m_iPlayersKilled = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
			return;

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			m_iPlayersKilled++;
			if ( m_iPlayersKilled >= 3 )
			{
				IncrementCount();
			}
		}
	}

private:
	int		m_iPlayersKilled;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillDefenseless, ACHIEVEMENT_TF_SOLDIER_KILL_DEFENSELESS, "TF_SOLDIER_KILL_DEFENSELESS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillWhileOnFire : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			if ( pLocalPlayer->m_Shared.InCond(TF_COND_BURNING) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillWhileOnFire, ACHIEVEMENT_TF_SOLDIER_KILL_ON_FIRE, "TF_SOLDIER_KILL_ON_FIRE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_FreezeTaunts : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Give opponents freezecams of you taunting
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_FreezeTaunts, ACHIEVEMENT_TF_SOLDIER_FREEZECAM_TAUNT, "TF_SOLDIER_FREEZECAM_TAUNT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_FreezeGibs : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Give opponents freezecams of you with X gibs onscreen
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_FreezeGibs, ACHIEVEMENT_TF_SOLDIER_FREEZECAM_GIBS, "TF_SOLDIER_FREEZECAM_GIBS", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillSpyKiller : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		m_vecBackstabbers.Purge();
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();
		if ( FStrEq( pszEvent, "teamplay_round_active" ) ||	FStrEq( pszEvent, "localplayer_respawn" ) )
		{
			m_vecBackstabbers.Purge();
		}
	}

	int	GetBackstabberIndex( CBaseEntity *pTarget )
	{
		for ( int i = 0; i < m_vecBackstabbers.Count(); i++ )
		{
			if ( m_vecBackstabbers[i].hSpy == pTarget )
				return i;
		}
		return -1;
	}

	void AddNewBackstabber( CBaseEntity *pTarget )
	{
		if ( !pTarget )
			return;

		// see if the target is already in our list or get a new index
		int iTargetIndex = GetBackstabberIndex( pTarget );
		if ( iTargetIndex == -1 )
		{
			iTargetIndex = m_vecBackstabbers.AddToTail();
		}

		m_vecBackstabbers[iTargetIndex].hSpy = pTarget;
		m_vecBackstabbers[iTargetIndex].flBackstabWindow = gpGlobals->curtime + 5.0f; // 5 seconds to kill the target
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pVictim == pLocalPlayer )
			return;

		CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_SPY ) && pVictim->InSameTeam( pLocalPlayer ) )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				AddNewBackstabber( pTFAttacker );
			}
		}
		else if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() )
		{
			int iTargetIndex = GetBackstabberIndex( pVictim );
			if ( iTargetIndex != -1 )
			{
				if ( m_vecBackstabbers[iTargetIndex].flBackstabWindow > gpGlobals->curtime )
				{
					// We killed him within 5 seconds of him backstabbing a buddy
					IncrementCount();
				}

				m_vecBackstabbers.Remove( iTargetIndex );
			}
		}
	}	

private:
	struct backstabber_t
	{
		EHANDLE hSpy;
		float flBackstabWindow;
	};
	CUtlVector<backstabber_t> m_vecBackstabbers;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillSpyKiller, ACHIEVEMENT_TF_SOLDIER_KILL_SPY_KILLER, "TF_SOLDIER_KILL_SPY_KILLER", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_GibGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			if ( event->GetInt( "death_flags" ) & TF_DEATH_GIBBED )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_GibGrind, ACHIEVEMENT_TF_SOLDIER_GIB_GRIND, "TF_SOLDIER_GIB_GRIND", 5 );



//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_DuoSoldierKills : public CBaseTFAchievement
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

			if ( pAttacker == C_BasePlayer::GetLocalPlayer() && pAssister->IsPlayerClass( TF_CLASS_SOLDIER ) )
			{
				// We are the attacker and the assist is from a soldier.
				IncrementCount();
				return;
			}

			if ( pAssister == C_BasePlayer::GetLocalPlayer() && pAttacker->IsPlayerClass( TF_CLASS_SOLDIER ) )
			{
				// We are the assister and the kill is from a soldier.
				IncrementCount();
				return;
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_DuoSoldierKills, ACHIEVEMENT_TF_SOLDIER_DUO_SOLDIER_KILLS, "TF_SOLDIER_DUO_SOLDIER_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_MVP : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_mvp" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_mvp" ) )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( event->GetInt( "player" ) ) );
			if ( !pTFPlayer )
				return;
			
			if ( pTFPlayer == C_BasePlayer::GetLocalPlayer() )
			{
				if ( CalcTeammateCount() >= 5 )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_MVP, ACHIEVEMENT_TF_SOLDIER_MVP, "TF_SOLDIER_MVP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_ThreeDominations : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		nDominations = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_start" );
		ListenForGameEvent( "localplayer_respawn" );		
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( ( FStrEq( pszEvent, "localplayer_respawn" ) || FStrEq( pszEvent, "teamplay_round_start" ) ) )
		{
			nDominations = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Player died	
		if ( pVictim == C_BasePlayer::GetLocalPlayer() )
		{
			nDominations = 0;
			return;
		}

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			bool bDomination = event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION;

			if ( bDomination )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim )
				{
					nDominations++; 
				}
			}
		}

		if ( nDominations >= 3 )
		{
			IncrementCount();
		}
	}

private:

	int nDominations;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_ThreeDominations, ACHIEVEMENT_TF_SOLDIER_THREE_DOMINATIONS, "TF_SOLDIER_THREE_DOMINATIONS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_RideTheCart : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "controlpoint_starttouch" );
		ListenForGameEvent( "controlpoint_endtouch" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_round_start" );
		ResetStatus();
		m_nRideTime = 30;
	}

	void ResetStatus()
	{
		ClearThink();
		m_bIsThinking = false;
		m_fRideStartTime = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// Pushed the cart onto the cap before time
		if ( Q_strcmp( event->GetName(), "teamplay_round_win" ) == 0 && m_bIsThinking )
		{
			ResetStatus();
		}

		if ( Q_strcmp( event->GetName(), "teamplay_round_start" ) == 0 && m_bIsThinking )
		{
			ResetStatus();
		}

		if ( !TFGameRules() || TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT )
			return;

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		C_TFTeam *pTeam = GetGlobalTFTeam( pLocalPlayer->GetTeamNumber() );
		if ( pTeam->GetRole() != TEAM_ROLE_ATTACKERS )
			return;

		// Player in cart trigger.  Register the time.
		if ( Q_strcmp( event->GetName(), "controlpoint_starttouch" ) == 0 && !m_bIsThinking )
		{
			int iPlayerIndex = event->GetInt( "player", 0 );

			if ( iPlayerIndex == GetLocalPlayerIndex() )
			{
				m_fRideStartTime = gpGlobals->curtime + (float)m_nRideTime;
				SetNextThink( m_nRideTime );
				m_bIsThinking = true;
			}
		}

		// Reset conditions:
		// Left the trigger
		if ( Q_strcmp( event->GetName(), "controlpoint_endtouch" ) == 0 && m_bIsThinking )
		{
			int iPlayerIndex = event->GetInt( "player", 0 );

			if ( iPlayerIndex == GetLocalPlayerIndex() )
			{
				ResetStatus();
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Died in the trigger
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pVictim == pLocalPlayer && m_bIsThinking )
		{
			ResetStatus();
		}
	}

	virtual void Think( void )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !TFGameRules() )
			return;

		if ( pLocalPlayer && pLocalPlayer->IsAlive() && m_fRideStartTime <= gpGlobals->curtime )
		{
			IncrementCount();
		}
	}

private:

	int		m_nRideTime;
	float	m_fRideStartTime;
	bool	m_bIsThinking;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_RideTheCart, ACHIEVEMENT_TF_SOLDIER_RIDE_THE_CART, "TF_SOLDIER_RIDE_THE_CART", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillTwentyFromAbove : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );

		if ( pLocalPlayer )
		{
			int iWeaponID = event->GetInt( "weaponid" );
			bool bRocketLauncherUsed = (iWeaponID == TF_WEAPON_ROCKETLAUNCHER) || (iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT);
			if ( pAttacker == pLocalPlayer && bRocketLauncherUsed )
			{	
				// Determine height difference - probably need to factor in distance, too
				Vector vecAttacker = pAttacker->GetAbsOrigin();
				Vector vecVictim = pVictim->GetAbsOrigin();
				Vector vecDelta = vecAttacker - vecVictim;

				// Msg( "ZDelta: %f \n", vecDelta.z );
				if ( vecDelta.z >= 92 )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillTwentyFromAbove, ACHIEVEMENT_TF_SOLDIER_KILL_TWENTY_FROM_ABOVE, "TF_SOLDIER_KILL_TWENTY_FROM_ABOVE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_KillFiveStunned : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		CBasePlayer *pLocalPlayer = C_TFPlayer::GetLocalPlayer();
		if ( !pLocalPlayer )
			return;

		if ( pLocalPlayer == pAttacker )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->m_Shared.InCond( TF_COND_STUNNED ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_KillFiveStunned, ACHIEVEMENT_TF_SOLDIER_KILL_FIVE_STUNNED, "TF_SOLDIER_KILL_FIVE_STUNNED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_DefendCapThirtyTimes : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 30 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_capture_blocked" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_capture_blocked" ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				int iBlocker = event->GetInt( "blocker", 0 );
				if ( iBlocker == GetLocalPlayerIndex() )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_DefendCapThirtyTimes, ACHIEVEMENT_TF_SOLDIER_DEFEND_CAP_THIRTY_TIMES, "TF_SOLDIER_DEFEND_CAP_THIRTY_TIMES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSoldier_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 5, ACHIEVEMENT_TF_SOLDIER_START_RANGE, ACHIEVEMENT_TF_SOLDIER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_AchieveProgress1, ACHIEVEMENT_TF_SOLDIER_ACHIEVE_PROGRESS1, "TF_SOLDIER_ACHIEVE_PROGRESS1", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSoldier_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 11, ACHIEVEMENT_TF_SOLDIER_START_RANGE, ACHIEVEMENT_TF_SOLDIER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_AchieveProgress2, ACHIEVEMENT_TF_SOLDIER_ACHIEVE_PROGRESS2, "TF_SOLDIER_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSoldier_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 17, ACHIEVEMENT_TF_SOLDIER_START_RANGE, ACHIEVEMENT_TF_SOLDIER_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_AchieveProgress3, ACHIEVEMENT_TF_SOLDIER_ACHIEVE_PROGRESS3, "TF_SOLDIER_ACHIEVE_PROGRESS3", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_ParachuteKillGroup : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		ResetTracking();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "parachute_deploy" );
		ListenForGameEvent( "parachute_holster" );
	}

	void ResetTracking( void )
	{
		m_nKills = 0;
		m_bParachuteDeployed = false;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "parachute_deploy" ) )
		{
			if ( event->GetInt( "index" ) == GetLocalPlayerIndex() )
			{
				ResetTracking();
				m_bParachuteDeployed = true;
			}
		}
		else if ( FStrEq( pszEvent, "parachute_holster" ) )
		{
			if ( event->GetInt( "index" ) == GetLocalPlayerIndex() )
			{
				ResetTracking();
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim )
		{
			// Local player died	
			if ( pVictim == C_BasePlayer::GetLocalPlayer() )
			{
				ResetTracking();
			}
			else if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
			{
				if ( m_bParachuteDeployed )
				{
					m_nKills++;
					if ( m_nKills >= 3 )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	int m_nKills;
	bool m_bParachuteDeployed;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_ParachuteKillGroup, ACHIEVEMENT_TF_SOLDIER_PARACHUTE_KILL_GROUP, "TF_SOLDIER_PARACHUTE_KILL_GROUP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_ParachuteDistance : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1233619 );
		SetStoreProgressInSteam( true );
		ResetTracking();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "parachute_deploy" );
		ListenForGameEvent( "parachute_holster" );
	}

	void ResetTracking( void )
	{
		ClearThink();
		m_flLastZ = 0.0;
	}

	void CheckHeight( void )
	{
		CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			float flCurrentZ = pLocalPlayer->GetAbsOrigin().z;
			float flDela = m_flLastZ - flCurrentZ;

			// make sure we've fallen....we may have been pushed upwards
			if ( flDela > 0.0f )
			{
				IncrementCount( (int)( flDela ) );
			}

			m_flLastZ = flCurrentZ;
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEvent = event->GetName();

		if ( FStrEq( pszEvent, "parachute_deploy" ) )
		{
			if ( event->GetInt( "index" ) == GetLocalPlayerIndex() )
			{
				CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
				if ( pLocalPlayer )
				{
					m_flLastZ = pLocalPlayer->GetAbsOrigin().z;
					SetNextThink( 0.1 );
				}
			}
		}
		else if ( FStrEq( pszEvent, "parachute_holster" ) )
		{
			if ( event->GetInt( "index" ) == GetLocalPlayerIndex() )
			{
				ResetTracking();
			}
		}
	}

	virtual void Think( void )
	{
		CheckHeight();
		SetNextThink( 0.1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Local player died	
		if ( pVictim == C_BasePlayer::GetLocalPlayer() )
		{
			ResetTracking();
		}
	}

private:
	float m_flLastZ;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_ParachuteDistance, ACHIEVEMENT_TF_SOLDIER_PARACHUTE_DISTANCE, "TF_SOLDIER_PARACHUTE_DISTANCE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_ParachuteKillParachute : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFAttacker && pTFVictim && ( pTFAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			if ( ( pTFAttacker->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) ) && ( pTFVictim->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_ParachuteKillParachute, ACHIEVEMENT_TF_SOLDIER_PARACHUTE_KILL_PARACHUTE, "TF_SOLDIER_PARACHUTE_KILL_PARACHUTE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_AirstrikeMaxClip : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_AirstrikeMaxClip, ACHIEVEMENT_TF_SOLDIER_AIRSTRIKE_MAX_CLIP, "TF_SOLDIER_AIRSTRIKE_MAX_CLIP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_AirstrikeGroupKill : public CBaseTFAchievement
{
public:
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	void ResetTracking( void )
	{
		m_iKilledDuringRJ = 0;
		m_bRocketJumping = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "rocket_jump" );
		ListenForGameEvent( "rocket_jump_landed" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ResetTracking();
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "teamplay_round_active" ) ||
			 FStrEq( pszEventName, "localplayer_respawn" ) )
		{
			ResetTracking();
		}
		else if ( FStrEq( pszEventName, "rocket_jump_landed" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
			if ( pLocalPlayer && ( pLocalPlayer->GetUserID() == iUserID ) )
			{
				ResetTracking();
			}
		}
		else if ( FStrEq( pszEventName, "rocket_jump" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
			if ( pLocalPlayer && ( pLocalPlayer->GetUserID() == iUserID ) )
			{
				m_iKilledDuringRJ = 0;
				m_bRocketJumping = true;
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( pVictim && ( pAttacker == CBasePlayer::GetLocalPlayer() ) )
		{
			if ( m_bRocketJumping && ( event->GetInt( "weaponid" ) == TF_WEAPON_ROCKETLAUNCHER ) )
			{
				CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				if ( pTFAttacker )
				{
					CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
					if ( pWeapon )
					{
						int iClipSizeOnKills = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iClipSizeOnKills, clipsize_increase_on_kill );
						if ( iClipSizeOnKills )
						{
							m_iKilledDuringRJ++;
							if ( m_iKilledDuringRJ == 3 )
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
	bool	m_bRocketJumping;
	int		m_iKilledDuringRJ;
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_AirstrikeGroupKill, ACHIEVEMENT_TF_SOLDIER_AIRSTRIKE_GROUP_KILL, "TF_SOLDIER_AIRSTRIKE_GROUP_KILL", 5 );

#endif // CLIENT_DLL
