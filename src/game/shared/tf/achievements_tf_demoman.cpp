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
#include "movevars_shared.h"

//======================================================================================================================================
// DEMOMAN ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillSoldierGrind : public CBaseTFAchievement
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
				if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SOLDIER ) )
				{
					// we killed a soldier
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillSoldierGrind, ACHIEVEMENT_TF_DEMOMAN_KILL_SOLDIER_GRIND, "TF_DEMOMAN_KILL_SOLDIER_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DestroyBuildingsWithMedic : public CBaseTFAchievement
{
	DECLARE_CLASS( CAchievementTFDemoman_DestroyBuildingsWithMedic, CBaseTFAchievement );

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iBuildingsDestroyed = 0;
	}

	virtual void ListenForEvents()
	{
		BaseClass::ListenForEvents();
		ListenForGameEvent( "player_invulned" );
		ListenForGameEvent( "object_destroyed" );
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
				m_iBuildingsDestroyed = 0;
			}
		}
		else if ( Q_strcmp( event->GetName(), "object_destroyed" ) == 0 )
		{
			int iAttackerIndex = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			if ( iAttackerIndex > 0 )
			{
				CBaseEntity *pAttacker = UTIL_PlayerByIndex( iAttackerIndex );
				C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pTFPlayer && pTFPlayer == pAttacker )
				{
					if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pTFPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED ) ||
						pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
					{
						m_iBuildingsDestroyed++;
						if ( m_iBuildingsDestroyed >= 5 )
						{
							IncrementCount();
						}
					}
				}
			}
		}
		else
		{
			BaseClass::FireGameEvent_Internal( event );
		}
	}

private:
	int		m_iBuildingsDestroyed;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DestroyBuildingsWithMedic, ACHIEVEMENT_TF_DEMOMAN_DESTROY_BUILDINGS_WITH_MEDIC, "TF_DEMOMAN_DESTROY_BUILDINGS_WITH_MEDIC", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DecapitateCloakedSpy : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
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
			if ( customdmg == TF_DMG_CUSTOM_DECAPITATION )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DecapitateCloakedSpy, ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_CLOAKED_SPY, "TF_DEMOMAN_DECAPITATE_CLOAKED_SPY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillWithDirectPipe : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// Kill 50 enemies with direct hits from the pipebomb launcher
	// The server (tf_player) awards this achievement - no code necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillWithDirectPipe, ACHIEVEMENT_TF_DEMOMAN_KILL_X_WITH_DIRECTPIPE, "TF_DEMOMAN_KILL_X_WITH_DIRECTPIPE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_BounceAndKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
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
			int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			CBasePlayer *pAttacker = UTIL_PlayerByIndex( iAttacker );
			if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() )
			{
				int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				CBasePlayer *pVictim = UTIL_PlayerByIndex( iVictim );
				if ( pVictim )
				{
					bool bVictimGrounded = pVictim->GetFlags() & FL_ONGROUND;
					if ( bVictimGrounded )
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
		if ( pVictim == C_BasePlayer::GetLocalPlayer() )
		{
			m_BouncedPlayers.RemoveAll();
			return;
		}
		
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && !( pTFVictim->GetFlags() & FL_ONGROUND ) )
			{
				int iIndex = m_BouncedPlayers.Find( pTFVictim->GetUserID() );
				if ( iIndex != m_BouncedPlayers.InvalidIndex() )
				{
					if ( gpGlobals->curtime < m_BouncedPlayers[iIndex] + 4.0f )
					{
						IncrementCount();
					}
				}
			}
		}
	}

	CUtlMap< int, float > m_BouncedPlayers; // userID and most recent time they were bounced
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_BounceAndKill, ACHIEVEMENT_TF_DEMOMAN_BOUNCE_AND_KILL, "TF_DEMOMAN_BOUNCE_AND_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DecapitatePlayers : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pLocalPlayer != pAttacker )
			return;

		int customdmg = event->GetInt( "customkill" );
		if ( customdmg == TF_DMG_CUSTOM_DECAPITATION )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DecapitatePlayers, ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_PLAYERS, "TF_DEMOMAN_DECAPITATE_PLAYERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DecapitatePlayersFast : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_iTimelyDecapitations = 0;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iTimelyDecapitations = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pLocalPlayer != pAttacker )
			return;

		int customdmg = event->GetInt( "customkill" );
		if ( customdmg == TF_DMG_CUSTOM_DECAPITATION )
		{
			if ( (m_iTimelyDecapitations == 0) || ((m_iTimelyDecapitations > 0) && (gpGlobals->curtime < m_flLastDecapTime + 10.f)) )
			{
				m_iTimelyDecapitations++;
			}
			else
			{
				m_iTimelyDecapitations = 1;
			}

			m_flLastDecapTime = gpGlobals->curtime;

			if ( m_iTimelyDecapitations == 4 )
			{
				IncrementCount();
			}
		}
	}

	int m_iTimelyDecapitations;
	float m_flLastDecapTime;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DecapitatePlayersFast, ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_PLAYERS_FAST, "TF_DEMOMAN_DECAPITATE_PLAYERS_FAST", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSoldier_DuoDemomanKills : public CBaseTFAchievement
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

			if ( pAttacker == C_BasePlayer::GetLocalPlayer() && pAssister->IsPlayerClass( TF_CLASS_DEMOMAN ) )
			{
				// We are the attacker and the assist is from a demoman.
				IncrementCount();
				return;
			}

			if ( pAssister == C_BasePlayer::GetLocalPlayer() && pAttacker->IsPlayerClass( TF_CLASS_DEMOMAN ) )
			{
				// We are the assister and the kill is from a demoman.
				IncrementCount();
				return;
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSoldier_DuoDemomanKills, ACHIEVEMENT_TF_DEMOMAN_DUO_DEMOMAN_KILLS, "TF_DEMOMAN_DUO_DEMOMAN_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillTwoDuringStickyJump : public CBaseTFAchievement
{
	// While airborne after a sticky-jump, kill at least two enemy players

	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "sticky_jump" );
		ListenForGameEvent( "sticky_jump_landed" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_iKilledDuringStickyJump = 0;
		m_bStickyJumping = false;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iKilledDuringStickyJump = 0;
			m_bStickyJumping = false;
		}
		else if ( FStrEq( event->GetName(), "sticky_jump_landed" ) )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				C_TFPlayer *pJumper = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );

				if ( pJumper && ( pJumper == C_BasePlayer::GetLocalPlayer() ) )
				{
					m_iKilledDuringStickyJump = 0;
					m_bStickyJumping = false;
				}
			}
			
			return;
		}
		else if ( FStrEq( event->GetName(), "sticky_jump" ) )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				C_TFPlayer *pJumper = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );

				if (  pJumper && ( pJumper == C_BasePlayer::GetLocalPlayer() ) )
				{
					m_iKilledDuringStickyJump = 0;
					m_bStickyJumping = true;
				}
			}

			return;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker && ( pAttacker == C_BasePlayer::GetLocalPlayer() ) )
		{
			if ( m_bStickyJumping )
			{
				m_iKilledDuringStickyJump++;

				if ( m_iKilledDuringStickyJump == 2 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	bool	m_bStickyJumping;
	int		m_iKilledDuringStickyJump;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillTwoDuringStickyJump, ACHIEVEMENT_TF_DEMOMAN_KILL_TWO_DURING_STICKYJUMP, "TF_DEMOMAN_KILL_TWO_DURING_STICKYJUMP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillPlayerAfterTeleport : public CBaseTFAchievement
{
	// Kill an enemy player within X seconds of them teleporting in

	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( !pVictim || !pVictim->IsPlayer() )
			return;
		
		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_PIPEBOMB )
		{
			if ( pTFVictim->m_Shared.InCond( TF_COND_TELEPORTED ) && ( gpGlobals->curtime - pTFVictim->m_Shared.GetTimeTeleEffectAdded() <= 5.0 ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillPlayerAfterTeleport, ACHIEVEMENT_TF_DEMOMAN_KILL_PLAYER_AFTER_TP, "TF_DEMOMAN_KILL_PLAYER_AFTER_TP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DominateEngineerThreeTimes : public CBaseTFAchievement
{
	// Dominate three Engineers

	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		bool bDomination = event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION;
		if ( bDomination == true )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				IncrementCount(); 
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DominateEngineerThreeTimes, ACHIEVEMENT_TF_DEMOMAN_DOMINATE_THREE_ENGINEERS, "TF_DEMOMAN_DOMINATE_THREE_ENGINEERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillBuildingDirectHit : public CBaseTFAchievement
{
	// Kill an Engineer building that you can't see with a direct hit from your Grenade Launcher
	// Server awards this achievement (tf_obj.cpp) - no other code necessary

	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillBuildingDirectHit, ACHIEVEMENT_TF_DEMOMAN_KILL_BUILDING_DIRECT_HIT, "TF_DEMOMAN_KILL_BUILDING_DIRECT_HIT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_MeleeKillWhileJumping : public CBaseTFAchievement
{
	// Get a melee kill while in sticky jump

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "sticky_jump" );
		ListenForGameEvent( "sticky_jump_landed" );
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		m_bStickyJumping = false;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) ||
			FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_bStickyJumping = false;
		}
		else if ( FStrEq( event->GetName(), "sticky_jump_landed" ) )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				C_TFPlayer *pJumper = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );

				if ( pJumper && ( pJumper == C_BasePlayer::GetLocalPlayer() ) )
				{
					m_bStickyJumping = false;
				}
			}

			return;
		}
		else if ( FStrEq( event->GetName(), "sticky_jump" ) )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				C_TFPlayer *pJumper = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );

				if (  pJumper && ( pJumper == C_BasePlayer::GetLocalPlayer() ) )
				{
					m_bStickyJumping = true;
				}
			}

			return;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		if ( pAttacker && ( pAttacker == C_BasePlayer::GetLocalPlayer() ) )
		{
			if ( m_bStickyJumping )
			{
				if ( event->GetInt( "weaponid" ) == TF_WEAPON_BOTTLE || event->GetInt( "weaponid" ) == TF_WEAPON_SWORD )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	bool	m_bStickyJumping;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_MeleeKillWhileJumping, ACHIEVEMENT_TF_DEMOMAN_MELEE_KILL_WHILE_STICKYJUMPING, "TF_DEMOMAN_MELEE_KILL_WHILE_STICKYJUMPING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillEngiSentryDispenser : public CBaseTFAchievement
{
	// Kill an Engineer, his Sentry and Dispenser, with one sticky detonation

	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetDefLessFunc( m_Engineers );
		SetDefLessFunc( m_SentryDestroyed );
		SetDefLessFunc( m_DispenserDestroyed );
		ResetTracking();
	}

	virtual void ListenForEvents()
	{
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
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_PIPEBOMB )
			{
				int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
				CBaseEntity *pAttacker = UTIL_PlayerByIndex( iAttacker );
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

				if ( !pLocalPlayer )
					return;

				if ( pLocalPlayer != pAttacker )
					return;

				int iEngineer = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				int iObject = event->GetInt( "objecttype" );

				if ( iObject == OBJ_SENTRYGUN )
				{
					int iIndex = m_SentryDestroyed.Find( iEngineer );
					if ( iIndex != m_SentryDestroyed.InvalidIndex() )
					{
						// already tracking them, update the time
						m_SentryDestroyed[iIndex] = gpGlobals->curtime;
					}
					else
					{
						m_SentryDestroyed.Insert( iEngineer, gpGlobals->curtime );
					}

					CheckGoalMet();
				}
				if ( iObject == OBJ_DISPENSER )
				{
					int iIndex = m_DispenserDestroyed.Find( iEngineer );
					if ( iIndex != m_DispenserDestroyed.InvalidIndex() )
					{
						// already tracking them, update the time
						m_DispenserDestroyed[iIndex] = gpGlobals->curtime;
					}
					else
					{
						m_DispenserDestroyed.Insert( iEngineer, gpGlobals->curtime );
					}

					CheckGoalMet();
				}
			}
		}
	}

	void ResetTracking( void )
	{
		m_Engineers.RemoveAll();
		m_SentryDestroyed.RemoveAll();
		m_DispenserDestroyed.RemoveAll();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_PIPEBOMB )
		{
			CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( !pLocalPlayer )
				return;

			if ( !pVictim || !pVictim->IsPlayer() )
				return;

			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				int iEngineer = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
				m_Engineers.InsertOrReplace( iEngineer, gpGlobals->curtime );

				CheckGoalMet();
			}
		}
	}

	void CheckGoalMet( void )
	{
		FOR_EACH_MAP_FAST ( m_Engineers, numEngineers )
		{
			int iEngineer = m_Engineers.Key( numEngineers );

			// Destroyed their Sentry?
			int iSentry = m_SentryDestroyed.Find( iEngineer );
			if ( iSentry != m_SentryDestroyed.InvalidIndex() )
			{
				// Destroyed their Dispenser?
				int iDispenser = m_DispenserDestroyed.Find( iEngineer );
				if ( iDispenser != m_DispenserDestroyed.InvalidIndex() )
				{
					// All three at the same time?
// 					if ( ( m_Engineers[numEngineers] == m_SentryDestroyed[iSentry] ) &&
// 						( m_Engineers[numEngineers] == m_DispenserDestroyed[iDispenser] ) )
					if ( ( gpGlobals->curtime < m_SentryDestroyed[iSentry] + 0.1f ) &&
						( gpGlobals->curtime < m_DispenserDestroyed[iDispenser] + 0.1f ) && 
						( gpGlobals->curtime < m_Engineers[numEngineers] + 0.1f ) )
					{
						IncrementCount();
					}
				}
			}
		}
	}

private:
	CUtlMap< int, float > m_SentryDestroyed;	// Engineer userID and time sentry was destroyed
	CUtlMap< int, float > m_DispenserDestroyed;	// Engineer userID and time dispenser was destroyed
	CUtlMap< int, float > m_Engineers;			// Engineers we've killed and the time
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillEngiSentryDispenser, ACHIEVEMENT_TF_DEMOMAN_KILL_ENGI_SENTRY_DISPENSER, "TF_DEMOMAN_KILL_ENGI_SENTRY_DISPENSER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DecapitateEqualizer : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			CTFShovel *pShovel = dynamic_cast<CTFShovel*>( pTFVictim->GetActiveTFWeapon() );
			if ( pShovel && (pShovel->GetShovelType() == SHOVEL_DAMAGE_BOOST) )
			{
				int customdmg = event->GetInt( "customkill" );
				if ( customdmg == TF_DMG_CUSTOM_DECAPITATION )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DecapitateEqualizer, ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_EQUALIZER, "TF_DEMOMAN_DECAPITATE_EQUALIZER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DecapitateNemesis : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		if ( pTFVictim && pTFVictim->IsNemesisOfLocalPlayer() )
		{
			int customdmg = event->GetInt( "customkill" );
			if ( customdmg == TF_DMG_CUSTOM_DECAPITATION )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DecapitateNemesis, ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_NEMESIS, "TF_DEMOMAN_DECAPITATE_NEMESIS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DamageGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000000 );
		SetStoreProgressInSteam( true );
	}

	void OnPlayerStatsUpdate()
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_DEMOMAN );
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[TFSTAT_BLASTDAMAGE];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DamageGrind, ACHIEVEMENT_TF_DEMOMAN_DAMAGE_GRIND, "TF_DEMOMAN_DAMAGE_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillXCapping : public CBaseTFAchievement
{
	// Kill "x" players that are capping or pushing the cart with one detonation, "y" times
	// Server checks for this achievement - no code necessary
	
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillXCapping, ACHIEVEMENT_TF_DEMOMAN_KILL_X_CAPPING_ONEDET, "TF_DEMOMAN_KILL_X_CAPPING_ONEDET", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillXDefending : public CBaseTFAchievement
{
	// Kill "x" players that are defending
	// Server checks for this achievement - no code necessary

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillXDefending, ACHIEVEMENT_TF_DEMOMAN_KILL_X_DEFENDING, "TF_DEMOMAN_KILL_X_DEFENDING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DestroyBuildings : public CBaseTFAchievement
{
	// Destroy "x" buildings

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "object_destroyed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "object_destroyed" ) )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			CBaseEntity *pDestroyer = UTIL_PlayerByIndex( iIndex );
			C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pDestroyer == pTFPlayer )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DestroyBuildings, ACHIEVEMENT_TF_DEMOMAN_DESTROY_BUILDINGS_GRIND, "TF_DEMOMAN_DESTROY_BUILDINGS_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillXHeaviesAtFull : public CBaseTFAchievement
{
	// Kill 3 Heavies from full health with a single Sticky detonation
	// Server checks for this achievement - no code necessary

	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillXHeaviesAtFull, ACHIEVEMENT_TF_DEMOMAN_KILL_X_HEAVIES_FULLHP_ONEDET, "TF_DEMOMAN_KILL_X_HEAVIES_FULLHP_ONEDET", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillXScoutsPyros : public CBaseTFAchievement
{
	// Kill 25 Scouts or Pyros with the grenade launcher

	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim->IsPlayerClass( TF_CLASS_PYRO ) || pTFVictim->IsPlayerClass( TF_CLASS_SCOUT ) )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_DEMOMAN )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillXScoutsPyros, ACHIEVEMENT_TF_DEMOMAN_KILL_X_SCOUTS_PYROS, "TF_DEMOMAN_KILL_X_SCOUTS_PYROS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_TauntKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// was this a taunt kill?
		if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_TauntKill, ACHIEVEMENT_TF_DEMOMAN_TAUNT_KILL, "TF_DEMOMAN_TAUNT_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_ChargeKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// was this a taunt kill?
		if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_CHARGE_IMPACT )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_ChargeKill, ACHIEVEMENT_TF_DEMOMAN_CHARGE_KILL, "TF_DEMOMAN_CHARGE_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_CritSwordKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// was this a charge kill with the sword?
		int nDamageBits = event->GetInt( "damagebits" );
		if ( (nDamageBits & DMG_CRITICAL) && (event->GetInt( "customkill" ) == TF_DMG_CUSTOM_DECAPITATION) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_CritSwordKill, ACHIEVEMENT_TF_DEMOMAN_CRIT_SWORD_KILL, "TF_DEMOMAN_CRIT_SWORD_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_AirBurstKills : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER );
		SetGoal( 30 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// was this an air burst kill?
		if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_AIR_STICKY_BURST )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_AirBurstKills, ACHIEVEMENT_TF_DEMOMAN_AIR_BURST_KILLS, "TF_DEMOMAN_AIR_BURST_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_StickyJumpCap : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_ATTACKER_IS_PLAYER );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "controlpoint_endtouch" );
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "sticky_jump_landed" );
		ResetTracking();
	}

	void CheckGoalMet( void )
	{
		// If sticky_jump_landed and startcapture times occurred roughly together...
		float fTemp1 = 0;
		float fTemp2 = 0;
		if ( m_fLandTime > m_fCapStartTime )
		{
			fTemp1 = m_fLandTime;
			fTemp2 = m_fCapStartTime;
		}
		else
		{
			fTemp1 = m_fCapStartTime;
			fTemp2 = m_fLandTime;
		}

		float fDelta = fTemp1 - fTemp2;
		if ( fDelta <= 0.5f )
		{
			// Check if we successfully capped the point we landed on
			if ( m_bCapped )
			{
				IncrementCount();
			}
		}
	}

	void ResetTracking( void )
	{
		m_bCapped = false;
		m_fLandTime = 0;
		m_fCapStartTime = 0;
		m_iCapID = -1;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		// Only check CP maps
		if ( TFGameRules() && TFGameRules()->GetGameType() != TF_GAMETYPE_CP && TFGameRules()->GetGameType() != TF_GAMETYPE_ARENA )
			return;
		
		// Capture the time we land
		if ( FStrEq( event->GetName(), "sticky_jump_landed" ) )
		{
			int iIndex = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			if ( iIndex > 0 )
			{
				C_TFPlayer *pJumper = ToTFPlayer( UTIL_PlayerByIndex( iIndex ) );

				if ( pJumper && ( pJumper == C_BasePlayer::GetLocalPlayer() ) )
				{
					m_fLandTime = gpGlobals->curtime;
					CheckGoalMet();
				}
			}

			return;
		}
		// Capture the time we start capping
		else if ( FStrEq( event->GetName(), "teamplay_point_startcapture" ) )
		{
			const char *cappers = event->GetString( "cappers" );
			for ( int i = 0; i < Q_strlen( cappers ); i++ )
			{
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( !pLocalPlayer )
					return;

				int iPlayerIndex = (int) cappers[i];
				CTFPlayer *pCapper = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
				if ( pCapper && ( pCapper == C_BasePlayer::GetLocalPlayer() ) )
				{
					m_fCapStartTime = gpGlobals->curtime;
					m_iCapID = event->GetInt( "cp" );
					CheckGoalMet();
				}
			}
		}
		else if ( FStrEq( event->GetName(), "teamplay_point_captured" ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( !pLocalPlayer )
				return;
			
			int iTeam = event->GetInt( "team" );
			if ( iTeam == pLocalPlayer->GetTeamNumber() )
			{			
				const char *cappers = event->GetString( "cappers" );
				for ( int i = 0; i < Q_strlen( cappers ); i++ )
				{
					int iPlayerIndex = (int) cappers[i];
					CTFPlayer *pCapper = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
					if ( pCapper && ( pCapper == C_BasePlayer::GetLocalPlayer() ) )
					{
						if ( m_iCapID == event->GetInt( "cp" ) )
						{
							m_bCapped = true;
							CheckGoalMet();
						}
					}
				}
			}
		}
		else if ( FStrEq( event->GetName(), "controlpoint_endtouch" ) )
		{
			int iPlayerIndex = event->GetInt( "player", 0 );
			if ( iPlayerIndex == GetLocalPlayerIndex() )
			{
				// We failed to take the point
				ResetTracking();
			}
		}
		else if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			// Reset tracking at the start of a new round
			ResetTracking();
		}
	}

private:
	float	m_fLandTime;
	float	m_fCapStartTime;
	bool	m_bCapped;
	int		m_iCapID;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_StickyJumpCap, ACHIEVEMENT_TF_DEMOMAN_STICKYJUMP_CAP, "TF_DEMOMAN_STICKYJUMP_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_FreezeTaunt : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Give opponents freezecams of you taunting
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_FreezeTaunt, ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_SMILE, "TF_DEMOMAN_FREEZECAM_SMILE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_FreezeTauntRump : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Give opponents freezecams of you taunting
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_FreezeTauntRump, ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_RUMP, "TF_DEMOMAN_FREEZECAM_RUMP", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_EnvironmentalKill : public CBaseTFAchievement
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

		CTFPlayer *pLocalPlayer = ToTFPlayer( CBasePlayer::GetLocalPlayer() );
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
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_EnvironmentalKill, ACHIEVEMENT_TF_DEMOMAN_ENVIRONMENTAL_KILL, "TF_DEMOMAN_ENVIRONMENTAL_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_DestroyStickyBombs : public CBaseTFAchievement
{
	// Destroy X stickybombs with the Scottish Defender

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_DestroyStickyBombs, ACHIEVEMENT_TF_DEMOMAN_DESTROY_X_STICKYBOMBS, "TF_DEMOMAN_DESTROY_X_STICKYBOMBS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_StickyJumpDistance : public CBaseTFAchievement
{
	DECLARE_CLASS( CAchievementTFDemoman_StickyJumpDistance, CBaseTFAchievement );

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
		ResetTracking();
	}

	virtual bool IsActive()
	{
		return BaseClass::IsActive();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "sticky_jump" );
		ListenForGameEvent( "sticky_jump_landed" );
		ResetTracking();
	}

	void ResetTracking( void )
	{
		ClearThink();
		m_vecStartJump = vec3_origin;
	}

	void CheckJump( void )
	{
		CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( pLocalTFPlayer )
		{
			// someone has messed with gravity, so reset our tracking
			if ( GetCurrentGravity() != 800.0f )
			{
				ResetTracking();
				return;
			}

			Vector vecCurrent = pLocalTFPlayer->GetAbsOrigin();
			vecCurrent.z = 0;
			float flDistSqr = (vecCurrent - m_vecStartJump).Length2DSqr();
			if ( flDistSqr > (2048 * 2048) )
			{
				IncrementCount();
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
		else if ( FStrEq( pszEvent, "sticky_jump" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( pLocalTFPlayer && pLocalTFPlayer->GetUserID() == iUserID )
			{
				m_vecStartJump = pLocalTFPlayer->GetAbsOrigin();
				m_vecStartJump.z = 0;
				SetNextThink( 0.1 );
			}
		}
		else if ( FStrEq( pszEvent, "sticky_jump_landed" ) )
		{
			int iUserID = event->GetInt( "userid" );

			CTFPlayer *pLocalTFPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( pLocalTFPlayer && pLocalTFPlayer->GetUserID() == iUserID )
			{
				CheckJump();
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
		CheckJump();
		SetNextThink( 0.1 );
	}

private:
	Vector m_vecStartJump;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_StickyJumpDistance, ACHIEVEMENT_TF_DEMOMAN_STICKJUMP_DISTANCE, "TF_DEMOMAN_STICKJUMP_DISTANCE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_Kill3WithDetonation : public CBaseTFAchievement
{
	// Destroy 3 players with a single pipebomb

	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_Kill3WithDetonation, ACHIEVEMENT_TF_DEMOMAN_KILL3_WITH_DETONATION, "TF_DEMOMAN_KILL3_WITH_DETONATION", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillXSappingSpies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_sapped_object" );
		m_hTargets.Purge();
	}

	int FindSpyInList( int iUserID )
	{
		for ( int i = m_hTargets.Count() - 1; i >= 0; i-- )
		{
			if ( m_hTargets[i].flTime < gpGlobals->curtime )
			{
				// time has run out on this one
				m_hTargets.Remove( i );
				continue;
			}

			if ( m_hTargets[i].nSpyUserID == iUserID )
				return i;
		}

		return -1;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) )
			{
				int iIndex = FindSpyInList( pTFVictim->GetUserID() );
				if ( iIndex != -1 )
				{
					IncrementCount();
					m_hTargets.Remove( iIndex );
				}
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_sapped_object" ) )
		{
			int nUserID = event->GetInt( "userid" );
			int iIndex = FindSpyInList( nUserID );
			if ( iIndex == -1 )
			{
				iIndex = m_hTargets.AddToTail();
				m_hTargets[iIndex].nSpyUserID = nUserID;
			}
			m_hTargets[iIndex].flTime = gpGlobals->curtime + 5.0;
		}
	}

private:
	struct targets_t
	{
		int nSpyUserID;
		float flTime;
	};

	CUtlVector<targets_t> m_hTargets;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillXSappingSpies, ACHIEVEMENT_TF_DEMOMAN_KILLXSAPPINGSPIES, "TF_DEMOMAN_KILLXSAPPINGSPIES", 5 );


//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_Kill3WithPipeSetup : public CBaseTFAchievement
{
	// Kill 3 players in separate explosions without placing new sticky bombs

	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ResetCounts();
	}

	void ResetCounts()
	{
		m_iConsecutiveKillsWithoutRefiring = 0;
		m_flPrevKillTime = 0;
		m_iPipebombCount = 0;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_PIPEBOMB )
		{
			if ( !m_iConsecutiveKillsWithoutRefiring )
			{
				// We're starting.
				m_iConsecutiveKillsWithoutRefiring = 1;
				m_flPrevKillTime = gpGlobals->curtime;
				m_iPipebombCount = pLocalPlayer->GetNumActivePipebombs();
				SetNextThink( 0.1 );
			}
			else if ( (gpGlobals->curtime - m_flPrevKillTime) > 0.2 )		// Make sure it wasn't in the same detonation
			{
				m_iConsecutiveKillsWithoutRefiring++;
				m_flPrevKillTime = gpGlobals->curtime;
				if ( m_iConsecutiveKillsWithoutRefiring >= 3 )
				{
					IncrementCount();
				}
			}
		}
	}

	virtual void Think( void )
	{
		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		int nPipes = pLocalPlayer->GetNumActivePipebombs();
		if ( m_iPipebombCount < nPipes )
		{
			// They laid new pipes. We're done.
			ResetCounts();
		}
		else
		{
			m_iPipebombCount = nPipes;
			SetNextThink( 0.1 );
		}
	}

private:
	int		m_iPipebombCount;
	int		m_iConsecutiveKillsWithoutRefiring;
	float	m_flPrevKillTime;
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_Kill3WithPipeSetup, ACHIEVEMENT_TF_DEMOMAN_KILL3_WITH_PIPE_SETUPS, "TF_DEMOMAN_KILL3_WITH_PIPE_SETUPS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFDemoman_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 5, ACHIEVEMENT_TF_DEMOMAN_START_RANGE, ACHIEVEMENT_TF_DEMOMAN_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_AchieveProgress1, ACHIEVEMENT_TF_DEMOMAN_ACHIEVE_PROGRESS1, "TF_DEMOMAN_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFDemoman_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 11, ACHIEVEMENT_TF_DEMOMAN_START_RANGE, ACHIEVEMENT_TF_DEMOMAN_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_AchieveProgress2, ACHIEVEMENT_TF_DEMOMAN_ACHIEVE_PROGRESS2, "TF_DEMOMAN_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFDemoman_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 17, ACHIEVEMENT_TF_DEMOMAN_START_RANGE, ACHIEVEMENT_TF_DEMOMAN_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_AchieveProgress3, ACHIEVEMENT_TF_DEMOMAN_ACHIEVE_PROGRESS3, "TF_DEMOMAN_ACHIEVE_PROGRESS3", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_ParachuteKillGroup : public CBaseTFAchievement
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
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_ParachuteKillGroup, ACHIEVEMENT_TF_DEMOMAN_PARACHUTE_KILL_GROUP, "TF_DEMOMAN_PARACHUTE_KILL_GROUP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_ParachuteDistance : public CBaseTFAchievement
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
				IncrementCount( ( int )( flDela ) );
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
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_ParachuteDistance, ACHIEVEMENT_TF_DEMOMAN_PARACHUTE_DISTANCE, "TF_DEMOMAN_PARACHUTE_DISTANCE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_ParachuteKillParachute : public CBaseTFAchievement
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
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_ParachuteKillParachute, ACHIEVEMENT_TF_DEMOMAN_PARACHUTE_KILL_PARACHUTE, "TF_DEMOMAN_PARACHUTE_KILL_PARACHUTE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_KillPlayerYouDidntSee : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_KillPlayerYouDidntSee, ACHIEVEMENT_TF_DEMOMAN_KILL_PLAYER_YOU_DIDNT_SEE, "TF_DEMOMAN_KILL_PLAYER_YOU_DIDNT_SEE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_QuickKills : public CBaseTFAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "kill_refills_meter" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "kill_refills_meter" ) )
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
					if ( m_Times.Tail() - m_Times.Head() <= 6.0 )
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
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_QuickKills, ACHIEVEMENT_TF_DEMOMAN_QUICK_KILLS, "TF_DEMOMAN_QUICK_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFDemoman_ChargeKillChargingDemo : public CBaseTFAchievement
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
			if ( pTFAttacker->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) && pTFVictim->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFDemoman_ChargeKillChargingDemo, ACHIEVEMENT_TF_DEMOMAN_CHARGE_KILL_CHARGING_DEMO, "TF_DEMOMAN_CHARGE_KILL_CHARGING_DEMO", 5 );

#endif // CLIENT_DLL



