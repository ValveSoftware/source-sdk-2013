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
#include "c_tf_playerresource.h"
#include "tf_weapon_invis.h"

//======================================================================================================================================
// SPY ACHIEVEMENT PACK
//======================================================================================================================================

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabSnipers : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	void ResetTracking()
	{
		m_iSnipersKilled = 0;
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

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for backstabbing 3 snipers in a single life.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				m_iSnipersKilled++;
				if ( m_iSnipersKilled == 3 )
				{
					IncrementCount();
				}
			}
		}
	}

	int m_iSnipersKilled;

};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabSnipers, ACHIEVEMENT_TF_SPY_BACKSTAB_SNIPERS, "TF_SPY_BACKSTAB_SNIPERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_FreezecamFlick : public CBaseTFAchievement
{
	// Achievement for freeze camming on a cig flick w/ enemy corpse on screen.
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_FreezecamFlick, ACHIEVEMENT_TF_SPY_FREEZECAM_FLICK, "TF_SPY_FREEZECAM_FLICK", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabDisguisedSpy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for backstabbing a disguised spy.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				if ( pTFVictim->m_Shared.InCond( TF_COND_DISGUISED ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabDisguisedSpy, ACHIEVEMENT_TF_SPY_BACKSTAB_DISGUISED_SPY, "TF_SPY_BACKSTAB_DISGUISED_SPY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabDisguiseTarget : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Achievement for backstabbing the player you are disguised as.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		if ( pTFAttacker && pTFVictim && pTFVictim == pTFAttacker->m_Shared.GetDisguiseTarget() )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabDisguiseTarget, ACHIEVEMENT_TF_SPY_BACKSTAB_DISGUISE_TARGET, "TF_SPY_BACKSTAB_DISGUISE_TARGET", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyFastCap : public CBaseTFAchievement
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
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyFastCap, ACHIEVEMENT_TF_SPY_FAST_CAP, "TF_SPY_FAST_CAP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyDominateSniper : public CBaseTFAchievement
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
			bool bDomination = event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION;

			if ( bDomination == true )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

				if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) )
				{
					IncrementCount(); 
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyDominateSniper, ACHIEVEMENT_TF_SPY_DOMINATE_SNIPER, "TF_SPY_DOMINATE_SNIPER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBumpCloakedSpy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBumpCloakedSpy, ACHIEVEMENT_TF_SPY_BUMP_CLOAKED_SPY, "TF_SPY_BUMP_CLOAKED_SPY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyKillSpyWithKnife : public CBaseTFAchievement
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
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SPY ) && pTFVictim->GetActiveTFWeapon() )
			{
				if ( event->GetInt( "weaponid" ) == TF_WEAPON_KNIFE )
				{
					if ( pTFVictim->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_REVOLVER )
					{
						IncrementCount(); 
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyKillSpyWithKnife, ACHIEVEMENT_TF_SPY_KILL_SPY_WITH_KNIFE, "TF_SPY_KILL_SPY_WITH_KNIFE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyRevengeBackstab : public CBaseTFAchievement
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
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				bool bRevenge = event->GetInt( "death_flags" ) & TF_DEATH_REVENGE;

				if ( bRevenge == true )
				{
					IncrementCount(); 
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyRevengeBackstab, ACHIEVEMENT_TF_SPY_REVENGE_WITH_BACKSTAB, "TF_SPY_REVENGE_WITH_BACKSTAB", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyKnifeKillWhileJarated : public CBaseTFAchievement
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
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_KNIFE )
			{
				CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
				if ( pTFAttacker && pTFAttacker->m_Shared.InCond( TF_COND_URINE ) )
				{
					IncrementCount(); 
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyKnifeKillWhileJarated, ACHIEVEMENT_TF_SPY_KNIFE_KILL_WHILE_JARATED, "TF_SPY_KNIFE_KILL_WHILE_JARATED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				IncrementCount(); 
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabGrind, ACHIEVEMENT_TF_SPY_BACKSTAB_GRIND, "TF_SPY_BACKSTAB_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabMedicCharged : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabMedicCharged, ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_CHARGED, "TF_SPY_BACKSTAB_MEDIC_CHARGED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabDominatingEnemy : public CBaseTFAchievement
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
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim )
				{
					int nDominationsNeeded = ( event->GetInt( "revenge" ) > 0 ) ? 4 : 3;

					if ( g_TF_PR && g_TF_PR->GetActiveDominations( pTFVictim->entindex() ) >= nDominationsNeeded )
					{
						IncrementCount(); 
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabDominatingEnemy, ACHIEVEMENT_TF_SPY_BACKSTAB_DOMINATING_ENEMY, "TF_SPY_BACKSTAB_DOMINATING_ENEMY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabFriends : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
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
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabFriends, ACHIEVEMENT_TF_SPY_BACKSTAB_FRIENDS, "TF_SPY_BACKSTAB_FRIENDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyAmbassadorGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			if ( event->GetInt( "weaponid" ) == TF_WEAPON_REVOLVER )
			{
				if ( FStrEq( event->GetString( "weapon_logclassname", "" ), "ambassador" ) )
				{
					IncrementCount(); 
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyAmbassadorGrind, ACHIEVEMENT_TF_SPY_AMBASSADOR_GRIND, "TF_SPY_AMBASSADOR_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyAmbassadorSniperGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SNIPER ) )
			{
				if ( event->GetInt( "weaponid" ) == TF_WEAPON_REVOLVER )
				{
					if ( FStrEq( event->GetString( "weapon_logclassname", "" ), "ambassador" ) )
					{
						if ( IsHeadshot( event->GetInt( "customkill" ) ) )
						{
							IncrementCount(); 
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyAmbassadorSniperGrind, ACHIEVEMENT_TF_SPY_AMBASSADOR_SNIPER_GRIND, "TF_SPY_AMBASSADOR_SNIPER_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyAmbassadorScoutGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SCOUT ) )
			{
				if ( event->GetInt( "weaponid" ) == TF_WEAPON_REVOLVER )
				{
					if ( FStrEq( event->GetString( "weapon_logclassname", "" ), "ambassador" ) )
					{
						if ( IsHeadshot( event->GetInt( "customkill" ) ) )
						{
							IncrementCount(); 
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyAmbassadorScoutGrind, ACHIEVEMENT_TF_SPY_AMBASSADOR_SCOUT_GRIND, "TF_SPY_AMBASSADOR_SCOUT_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyMedicHealingKillEnemy : public CBaseTFAchievement
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
			for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

				// can't kill the enemy medic that's healing you for this one...should be a different enemy
				if ( pTFPlayer && ( pTFPlayer != pAttacker ) && ( pTFPlayer != pVictim ) )
				{
					// make sure they're not on our team
					if ( pAttacker->GetTeamNumber() != pTFPlayer->GetTeamNumber() )
					{
						// are they a medic that's healing me?
						if ( pTFPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pTFPlayer->MedicGetHealTarget() == pAttacker )
						{
							IncrementCount(); 
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyMedicHealingKillEnemy, ACHIEVEMENT_TF_SPY_MEDIC_HEALING_KILL_ENEMY, "TF_SPY_MEDIC_HEALING_KILL_ENEMY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabQuickKills : public CBaseTFAchievement
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
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				int iNewIndex = m_Times.AddToTail();
				m_Times[iNewIndex] = gpGlobals->curtime;

				// we only care about the last three times we backstabbed someone
				if ( m_Times.Count() > 3 )
				{
					m_Times.Remove( 0 );
				}

				if ( m_Times.Count() == 3 )
				{
					if ( m_Times.Tail() - m_Times.Head() <= 10.0 )
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
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabQuickKills, ACHIEVEMENT_TF_SPY_BACKSTAB_QUICK_KILLS, "TF_SPY_BACKSTAB_QUICK_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyTauntKill : public CBaseTFAchievement
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
			if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_FENCING )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyTauntKill, ACHIEVEMENT_TF_SPY_TAUNT_KILL, "TF_SPY_TAUNT_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabCappingEnemies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabCappingEnemies, ACHIEVEMENT_TF_SPY_BACKSTAB_CAPPING_ENEMIES, "TF_SPY_BACKSTAB_CAPPING_ENEMIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyKillCPDefenders : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 15 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyKillCPDefenders, ACHIEVEMENT_TF_SPY_KILL_CP_DEFENDERS, "TF_SPY_KILL_CP_DEFENDERS", 5 );

//----------------------------------------------------------------------------------------------------------------
//FYI I'm a spy
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_FYIMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );	
		m_hTargets.Purge();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "player_healedbymedic" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "player_healedbymedic" ) )
		{
			int iEntity = event->GetInt( "medic" );

			int iMedic = GetTargetIndex( UTIL_PlayerByIndex( iEntity ) );

			if ( iMedic == -1 )
			{
				AddNewTarget( UTIL_PlayerByIndex( iEntity ) );
			}
			else
			{
				m_hTargets[iMedic].flRemoveTime = gpGlobals->curtime + 5.0f;
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			int iMedic = GetTargetIndex( pVictim );

			if ( iMedic != -1 )
			{
				if ( m_hTargets[iMedic].flRemoveTime >= gpGlobals->curtime )
				{
					IncrementCount();
				}

				m_hTargets.Remove( iMedic );
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

		// see if the target is already in our list or get a new index
		int iMedic = GetTargetIndex( pTarget );
		if ( iMedic == -1 )
		{
			iMedic = m_hTargets.AddToTail();
		}

		m_hTargets[iMedic].hTarget = pTarget;
		m_hTargets[iMedic].flRemoveTime = gpGlobals->curtime + 5.0f;
	}

private:
	struct targets_t
	{
		EHANDLE hTarget;
		float flRemoveTime;
	};

	CUtlVector<targets_t> m_hTargets;
};

DECLARE_ACHIEVEMENT( CAchievementTFSpy_FYIMedic, ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_HEALING_YOU, "TF_SPY_BACKSTAB_MEDIC_HEALING_YOU", 5 );

//----------------------------------------------------------------------------------------------------------------
// Backstab a medic/heavy pair
// NOTE: Enough of this, this should be a shared achievement class.
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_KillMedicPair : public CBaseTFAchievement
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
		m_hTargets[iTargetIndex].flTimeToBeat = gpGlobals->curtime + 10.0f; // 10 seconds to kill the target
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( !pVictim || !pVictim->IsPlayer() )
			return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		int customkill = event->GetInt( "customkill" );

		if ( pLocalPlayer == pAttacker && customkill == TF_DMG_CUSTOM_BACKSTAB )
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
				if ( !pTFVictim->IsPlayerClass( TF_CLASS_MEDIC ) )
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
				else
				{
					pNewTarget = ToTFPlayer( pTFVictim->MedicGetHealTarget() );
					if ( pNewTarget )
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
DECLARE_ACHIEVEMENT( CAchievementTFSpy_KillMedicPair, ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_PAIR, "TF_SPY_BACKSTAB_MEDIC_PAIR", 5 );

//----------------------------------------------------------------------------------------------------------------
//Destroy 3 enemy sentries
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SapBuildingGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000 );
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

			if ( iIndex == GetLocalPlayerIndex() )
			{
				const char *pWeapon = event->GetString( "weapon" );

				if ( FStrEq( "obj_attachment_sapper", pWeapon ) ||
					 FStrEq( "snack_attack", pWeapon ) ||
					 FStrEq( "psapper", pWeapon ) ||
					 FStrEq( "recorder", pWeapon ) )
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SapBuildingGrind, ACHIEVEMENT_TF_SPY_SAPPER_GRIND, "TF_SPY_SAPPER_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill whoever triggered your feign death
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_FeignDeathKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );
		m_iKillerIndex = 0;
		m_flTriggerTime = gpGlobals->curtime;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_death" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "player_death" ) == 0 )
		{
			int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
			int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

			if ( iVictim == GetLocalPlayerIndex() )
			{
				//Someone popped my feign death
				if ( event->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH )
				{
					m_iKillerIndex = iAttacker;
					m_flTriggerTime = gpGlobals->curtime + 20.0f;
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );

			if ( pTFVictim && pTFVictim->entindex() == m_iKillerIndex && m_flTriggerTime >= gpGlobals->curtime )
			{
				IncrementCount();
			}
		}
	}

private:
	int m_iKillerIndex;
	float m_flTriggerTime;
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_FeignDeathKill, ACHIEVEMENT_TF_SPY_FEIGN_DEATH_KILL, "TF_SPY_FEIGN_DEATH_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
//Kill a sniper after breaking his shield
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_ShieldKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );	

		m_iBlockerIndex = 0;
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

			if ( GetLocalPlayerIndex() != iAttacker )
				return;

			m_iBlockerIndex = iBlocker;
			m_flAttackTime = gpGlobals->curtime + 10.0f;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( m_flAttackTime <= gpGlobals->curtime )
		{
			m_iBlockerIndex = 0;
			return;
		}

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( pTFVictim && pTFVictim->entindex() == m_iBlockerIndex && pAttacker == C_TFPlayer::GetLocalTFPlayer() )
		{
			IncrementCount();
		}
	}

private:
	int m_iBlockerIndex;
	float m_flAttackTime;
};

DECLARE_ACHIEVEMENT( CAchievementTFSpy_ShieldKill, ACHIEVEMENT_TF_SPY_BREAK_SHIELD_KILL_SNIPER, "TF_SPY_BREAK_SHIELD_KILL_SNIPER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_KillWorkingEngineer : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_KillWorkingEngineer, ACHIEVEMENT_TF_SPY_KILL_WORKING_ENGY, "TF_SPY_KILL_WORKING_ENGY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_TFSpySurviveBurning : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_bIsThinking = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_damaged" );
		ListenForGameEvent( "teamplay_round_start" );
		ListenForGameEvent( "localplayer_respawn" );
		m_bIsThinking = false;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "localplayer_respawn" ) || FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			ClearThink();
			m_bIsThinking = false;
			return;
		}

		// Have I taken damage?
		if ( FStrEq( pszEventName, "player_damaged" ))
		{
			int iDmgType = event->GetInt( "type" );

			// Did I survive the damage event?
			if ( pLocalPlayer->IsAlive() && !m_bIsThinking )
			{
				// Am I on fire?
				if ( (iDmgType & DMG_IGNITE) )
				{	
					// Am I cloaked?
					if ( pLocalPlayer->m_Shared.InCond ( TF_COND_STEALTHED ) )
					{					
						SetNextThink( 30 );
						m_bIsThinking = true;
					}
				}
			}
		}
	}

	virtual void Think( void )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && pLocalPlayer->IsAlive())
		{
			IncrementCount();
		}
	}

private:
	bool	m_bIsThinking;
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_TFSpySurviveBurning, ACHIEVEMENT_TF_SPY_SURVIVE_BURNING, "TF_SPY_SURVIVE_BURNING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabEnemySwitchPyro : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		ResetBackstabbedPlayers();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_spawn" );
		ListenForGameEvent( "localplayer_changeclass" );
		ListenForGameEvent( "localplayer_changeteam" );
	}

	void ResetBackstabbedPlayers( void )
	{
		m_BackstabbedPlayers.Purge();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// no progress in endround
		if ( !GameRulesAllowsAchievements() )
			return;

		if ( !IsLocalTFPlayerClass( TF_CLASS_SPY ) )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim && !pTFVictim->IsPlayerClass( TF_CLASS_PYRO ) ) // they can't already be a pyro for this one
				{
					int nUserID = pTFVictim->GetUserID();

					if ( m_BackstabbedPlayers.Find( nUserID ) == m_BackstabbedPlayers.InvalidIndex() )
					{
						// they're not in our list yet, add them
						m_BackstabbedPlayers.AddToTail( nUserID );
					}
				}
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_spawn" ) )
		{
			// no progress in endround
			if ( !GameRulesAllowsAchievements() )
				return;

			if ( !IsLocalTFPlayerClass( TF_CLASS_SPY ) )
				return;

			const int nUserID = event->GetInt( "userid" );
			int iIndex = m_BackstabbedPlayers.Find( nUserID );
			if ( iIndex != m_BackstabbedPlayers.InvalidIndex() )
			{
				const int nTeam = event->GetInt( "team" );
				if ( nTeam != GetLocalPlayerTeam() )
				{
					const int nClass = event->GetInt( "class" );
					if ( nClass == TF_CLASS_PYRO )
					{
						IncrementCount();
					}
				}

				m_BackstabbedPlayers.Remove( iIndex );
			}
		}
		else if ( FStrEq( pszEventName, "localplayer_changeclass" ) || 
				  FStrEq( pszEventName, "localplayer_changeteam" ) )
		{
			ResetBackstabbedPlayers();
		}
	}

private:
	CUtlVector< int > m_BackstabbedPlayers;
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabEnemySwitchPyro, ACHIEVEMENT_TF_SPY_BACKSTAB_ENEMY_SWITCH_PYRO, "TF_SPY_BACKSTAB_ENEMY_SWITCH_PYRO", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyBackstabEngySapBuilding : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		m_hTargets.Purge();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_sapped_object" );
	}

	int FindEngyInList( CBaseEntity *pTarget )
	{
		for ( int i = 0; i < m_hTargets.Count(); i++ )
		{
			if ( m_hTargets[i].hEngy == pTarget )
				return i;
		}
		return -1;
	}

	int CountSappedObjects( int iIndex )
	{
		int nCount = 0;

		if ( m_hTargets.IsValidIndex( iIndex ) ) 
		{
			if ( m_hTargets[iIndex].bSentry )
			{
				nCount++;
			}

			if ( m_hTargets[iIndex].bDispenser )
			{
				nCount++;
			}

			if ( m_hTargets[iIndex].bTele )
			{
				nCount++;
			}
		}

		return nCount;
	}

	void SetObjectSapped( int iIndex, int nType )
	{
		if ( m_hTargets.IsValidIndex( iIndex ) ) 
		{
			switch( nType )
			{
			case OBJ_SENTRYGUN:
				m_hTargets[iIndex].bSentry = true;
				break;
			case OBJ_DISPENSER:
				m_hTargets[iIndex].bDispenser = true;
				break;
			case OBJ_TELEPORTER:
				m_hTargets[iIndex].bTele = true;
				break;
			}
		}
	}

	void CheckAchievementEarned( void )
	{
		for ( int i = m_hTargets.Count() - 1; i >= 0; i-- )
		{
			if ( m_hTargets[i].flTimeToBeat < gpGlobals->curtime )
			{
				// time has run out on this one
				m_hTargets.Remove( i );
			}
			else
			{
				if ( CountSappedObjects( i ) >= 3 )
				{
					IncrementCount();
					m_hTargets.Purge();
					return;
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					int iIndex = FindEngyInList( pVictim );
					if ( iIndex == -1 )
					{
						iIndex = m_hTargets.AddToTail();
					}

					m_hTargets[iIndex].hEngy = pVictim;
					m_hTargets[iIndex].bSentry = false;
					m_hTargets[iIndex].bDispenser = false;
					m_hTargets[iIndex].bTele = false;
					m_hTargets[iIndex].flTimeToBeat = gpGlobals->curtime + 10.0;
				}
			}

			CheckAchievementEarned(); // checks the achievement list, but also cleans out old entries (based on flTimeToBeat)
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_sapped_object" ) )
		{
			int nUserID = event->GetInt( "userid" );

			CBasePlayer *pPlayer = UTIL_PlayerByIndex( engine->GetPlayerForUserID( nUserID ) );
			if ( pPlayer == C_BasePlayer::GetLocalPlayer() )
			{
				CBasePlayer *pEngy = UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "ownerid" ) ) );

				int iIndex = FindEngyInList( pEngy );
				if ( iIndex != -1 )
				{
					SetObjectSapped( iIndex, event->GetInt( "object" ) );
				}

				CheckAchievementEarned(); // checks the achievement list, but also cleans out old entries (based on flTimeToBeat)
			}
		}
	}

private:
	struct targets_t
	{
		EHANDLE hEngy;
		bool bSentry;
		bool bDispenser;
		bool bTele;
		float flTimeToBeat;
	};

	CUtlVector<targets_t> m_hTargets;
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyBackstabEngySapBuilding, ACHIEVEMENT_TF_SPY_BACKSTAB_ENGY_SAP_BUILDING, "TF_SPY_BACKSTAB_ENGY_SAP_BUILDING", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpySapBuildingBackstabEngy : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		m_hTargets.Purge();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_sapped_object" );
	}

	int FindEngyInList( CBaseEntity *pTarget )
	{
		for ( int i = 0; i < m_hTargets.Count(); i++ )
		{
			if ( m_hTargets[i].hEngy == pTarget )
				return i;
		}
		return -1;
	}

	void ValidateList( void )
	{
		for ( int i = m_hTargets.Count() - 1; i >= 0; i-- )
		{
			if ( m_hTargets[i].flTimeToBeat < gpGlobals->curtime )
			{
				// time has run out on this one
				m_hTargets.Remove( i );
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			int customkill = event->GetInt( "customkill" );
			if ( customkill == TF_DMG_CUSTOM_BACKSTAB )
			{
				CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
				if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					int iIndex = FindEngyInList( pVictim );
					if ( iIndex != -1 )
					{
						// they're in our list...is the time still valid?
						if ( m_hTargets[iIndex].flTimeToBeat > gpGlobals->curtime )
						{
							IncrementCount();
							m_hTargets.Purge();
							return;
						}
					}
				}
			}

			ValidateList();
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_sapped_object" ) )
		{
			int nUserID = event->GetInt( "userid" );

			CBasePlayer *pPlayer = UTIL_PlayerByIndex( engine->GetPlayerForUserID( nUserID ) );
			if ( pPlayer == C_BasePlayer::GetLocalPlayer() )
			{
				CBasePlayer *pEngy = UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "ownerid" ) ) );
				if ( pEngy )
				{
					int iIndex = FindEngyInList( pEngy );
					if ( iIndex == -1 )
					{
						iIndex = m_hTargets.AddToTail();
					}

					m_hTargets[iIndex].hEngy = pEngy;
					m_hTargets[iIndex].flTimeToBeat = gpGlobals->curtime + 5.0;
				}

				ValidateList();
			}
		}
	}

private:
	struct targets_t
	{
		EHANDLE hEngy;
		float flTimeToBeat;
	};

	CUtlVector<targets_t> m_hTargets;
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpySapBuildingBackstabEngy, ACHIEVEMENT_TF_SPY_SAP_BUILDING_BACKSTAB_ENGY, "TF_SPY_SAP_BUILDING_BACKSTAB_ENGY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SapperTeamwork : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		ResetData();
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_sapped_object" );
		ListenForGameEvent( "localplayer_changeteam" );
	}

	void ResetData( void )
	{
		m_hTeammates.Purge();

		m_nLocalPlayerTarget = -1;
		m_flLocalPlayerTime = -1;
	}

	int FindSpyInList( int userid )
	{
		for ( int i = 0; i < m_hTeammates.Count(); i++ )
		{
			if ( m_hTeammates[i].nSpy == userid )
				return i;
		}
		return -1;
	}

	void CheckAchievementEarned( void )
	{
		// has the local player sapped anything?
		if ( m_nLocalPlayerTarget == -1 && m_flLocalPlayerTime == -1 )
			return;

		// has it been longer than 3 seconds since the local player sapped something?
		if ( gpGlobals->curtime - m_flLocalPlayerTime > 3.0 )
		{
			// reset the local player target and time, this fixes the local player sapping something and 
			// then a teammate saps something > 3 seconds later (otherwise, the new entry would be removed in the for loop below)
			m_nLocalPlayerTarget = -1;
			m_flLocalPlayerTime = -1;
			return;
		}

		for ( int i = m_hTeammates.Count() - 1; i >= 0; i-- )
		{
			if ( m_hTeammates[i].nTarget != m_nLocalPlayerTarget ) // different guns
			{
				if ( fabs( m_hTeammates[i].flTime - m_flLocalPlayerTime ) <= 3.0 )
				{
					IncrementCount();
					ResetData();
					return;
				}
				else
				{
					m_hTeammates.Remove( i );
				}
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "player_sapped_object" ) )
		{
			if ( event->GetInt( "object" ) == OBJ_SENTRYGUN )
			{
				int nUserID = event->GetInt( "userid" );

				CTFPlayer *pSpy = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( nUserID ) ) );
				if ( pSpy && pSpy->GetTeamNumber() == GetLocalPlayerTeam() )
				{
					if ( pSpy == C_BasePlayer::GetLocalPlayer() )
					{
						m_nLocalPlayerTarget = event->GetInt( "ownerid" );
						m_flLocalPlayerTime = gpGlobals->curtime;
					}
					else
					{
						int iIndex = FindSpyInList( nUserID );
						if ( iIndex == -1 )
						{
							iIndex = m_hTeammates.AddToTail();
						}

						m_hTeammates[iIndex].nSpy = nUserID;
						m_hTeammates[iIndex].nTarget = event->GetInt( "ownerid" );
						m_hTeammates[iIndex].flTime = gpGlobals->curtime;
					}

					CheckAchievementEarned(); // checks the achievement list, but also cleans out old entries (based on time)
				}
			}
		}
		else if ( FStrEq( pszEventName, "localplayer_changeteam" ) )
		{
			ResetData();
		}
	}

private:
	struct teammates_t
	{
		int	nSpy; // who built the sapper
		int nTarget; // owner of the gun 
		float flTime; // time it was sapped
	};

	CUtlVector<teammates_t> m_hTeammates; // list of teammates who have sapped guns

	int m_nLocalPlayerTarget; // owner of the gun
	float m_flLocalPlayerTime; // time it was sapped
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SapperTeamwork, ACHIEVEMENT_TF_SPY_SAPPER_TEAMWORK, "TF_SPY_SAPPER_TEAMWORK", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_SpyCampPosition : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal( 1 );

		ResetData();
	}

	void ResetData()
	{
		m_hTargets.Purge();
	}

	virtual void ListenForEvents( void )
	{
		ListenForGameEvent( "localplayer_respawn" );
		ListenForGameEvent( "teamplay_round_active" );

		ResetData();
	}

	int FindEnemyInList( CBaseEntity *pTarget )
	{
		for ( int i = 0; i < m_hTargets.Count(); i++ )
		{
			if ( m_hTargets[i].hEnemy == pTarget )
				return i;
		}
		return -1;
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "localplayer_respawn" ) ||
			 FStrEq( pszEventName, "teamplay_round_active" ) )
		{
			ResetData();
		}
	}

	bool LocalPlayerHasMotionCloakEquipped( void )
	{
		CTFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pPlayer )
		{
			CTFWeaponInvis *pWeapon = (CTFWeaponInvis *) pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
			if ( pWeapon && pWeapon->HasMotionCloak() )
			{
				return true;
			}
		}

		return false;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( pAttacker == C_BasePlayer::GetLocalPlayer() )
		{
			if ( LocalPlayerHasMotionCloakEquipped() )
			{
				int iIndex = FindEnemyInList( pVictim );
				if ( iIndex != -1 )
				{
					// they're already in our list
					Vector vecOrigin = pVictim->GetAbsOrigin();
					
					if ( ( m_hTargets[iIndex].vecOrigin - vecOrigin ).LengthSqr() > (1024*1024) )
					{
						// we killed them too far away from our last position, so reset
						m_hTargets[iIndex].nKillCount = 1;
						m_hTargets[iIndex].vecOrigin = vecOrigin;
					}
					else
					{
						// they were close enough, so add to our kill count
						m_hTargets[iIndex].nKillCount++;
					}
				}
				else
				{
					// they're new to our list
					iIndex = m_hTargets.AddToTail();

					m_hTargets[iIndex].hEnemy = pVictim;
					m_hTargets[iIndex].nKillCount = 1;
					m_hTargets[iIndex].vecOrigin = pVictim->GetAbsOrigin();
				}

				if ( m_hTargets[iIndex].nKillCount >= 3 )
				{
					IncrementCount();
					ResetData();
				}
			}
			else
			{
				// Not using motion cloak anymore
				ResetData();
			}
		}
	}

private:
	struct targets_t
	{
		EHANDLE hEnemy;
		int nKillCount;
		Vector vecOrigin;
	};

	CUtlVector<targets_t> m_hTargets;

};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_SpyCampPosition, ACHIEVEMENT_TF_SPY_CAMP_POSITION, "TF_SPY_CAMP_POSITION", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSpy_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 5, ACHIEVEMENT_TF_SPY_START_RANGE, ACHIEVEMENT_TF_SPY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_AchieveProgress1, ACHIEVEMENT_TF_SPY_ACHIEVE_PROGRESS1, "TF_SPY_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSpy_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 11, ACHIEVEMENT_TF_SPY_START_RANGE, ACHIEVEMENT_TF_SPY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_AchieveProgress2, ACHIEVEMENT_TF_SPY_ACHIEVE_PROGRESS2, "TF_SPY_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFSpy_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 17, ACHIEVEMENT_TF_SPY_START_RANGE, ACHIEVEMENT_TF_SPY_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_AchieveProgress3, ACHIEVEMENT_TF_SPY_ACHIEVE_PROGRESS3, "TF_SPY_ACHIEVE_PROGRESS3", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFSpy_KillBackScatterScout : public CBaseTFAchievement
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
		if ( pTFVictim && pTFVictim->IsPlayerClass( TF_CLASS_SCOUT ) && ( pAttacker == C_TFPlayer::GetLocalTFPlayer() ) )
		{
			if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_BACKSTAB )
			{
				CTFWeaponBase *pWeapon = pTFVictim->GetActiveTFWeapon();
				if ( pWeapon && ( pWeapon->GetWeaponID() == TF_WEAPON_SCATTERGUN ) )
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
};
DECLARE_ACHIEVEMENT( CAchievementTFSpy_KillBackScatterScout, ACHIEVEMENT_TF_SPY_KILL_BACKSCATTER_SCOUT, "TF_SPY_KILL_BACKSCATTER_SCOUT", 5 );

#endif // CLIENT_DLL
