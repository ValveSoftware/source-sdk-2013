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

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BaseBurn : public CBaseTFAchievement
{
public:
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_ignited" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( FStrEq( event->GetName(), "player_ignited" ) )
		{
			int iPyro = event->GetInt( "pyro_entindex" );
			CBaseEntity *pPyro = UTIL_PlayerByIndex( iPyro );
			if ( pPyro == pLocalPlayer )
			{
				int iVictim = event->GetInt( "victim_entindex" );
				C_TFPlayer *pTFVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictim ) );
				if ( pTFVictim )
				{
					BurnedVictim( pPyro, pTFVictim, event );
				}
			}
		}
	}

	virtual void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
	}
};

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillPlayersWhileDead : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 15 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			if ( !pLocalPlayer->IsAlive() )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillPlayersWhileDead, ACHIEVEMENT_TF_PYRO_KILL_POSTDEATH, "TF_PYRO_KILL_POSTDEATH", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnFlagCarriers : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( pTFVictim->HasTheFlag() )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnFlagCarriers, ACHIEVEMENT_TF_PYRO_KILL_CARRIERS, "TF_PYRO_KILL_CARRIERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnCloakedSpies : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( pTFVictim->IsPlayerClass(TF_CLASS_SPY) && pTFVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnCloakedSpies, ACHIEVEMENT_TF_PYRO_REVEAL_SPIES, "TF_PYRO_REVEAL_SPIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnSpiesAsMe : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( pTFVictim->IsPlayerClass(TF_CLASS_SPY) && pTFVictim->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnSpiesAsMe, ACHIEVEMENT_TF_PYRO_BURN_SPIES_AS_YOU, "TF_PYRO_BURN_SPIES_AS_YOU", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_AxeKillsNoDeaths : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		m_iConsecutiveKills = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_active" );
		ListenForGameEvent( "localplayer_respawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "teamplay_round_active" ) )
		{
			m_iConsecutiveKills = 0;
		}
		else if ( FStrEq( event->GetName(), "localplayer_respawn" ) )
		{
			m_iConsecutiveKills = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer == pVictim )
		{
			m_iConsecutiveKills = 0;
		}
		else if ( pLocalPlayer == pAttacker && event->GetInt( "weaponid" ) == TF_WEAPON_FIREAXE )
		{
			m_iConsecutiveKills++;
			if ( m_iConsecutiveKills >= GetNumKillsNeeded() )
			{
				IncrementCount();
			}
		}	
	}

	virtual int GetNumKillsNeeded( void )
	{
		return 3;
	}

private:
	int m_iConsecutiveKills;
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_AxeKillsNoDeaths, ACHIEVEMENT_TF_PYRO_KILL_AXE_SMALL, "TF_PYRO_KILL_AXE_SMALL", 5 );

class CAchievementTFPyro_AxeKillsNoDeathsLarge : public CAchievementTFPyro_AxeKillsNoDeaths
{
	virtual int GetNumKillsNeeded( void )
	{
		return 6;
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_AxeKillsNoDeathsLarge, ACHIEVEMENT_TF_PYRO_KILL_AXE_LARGE, "TF_PYRO_KILL_AXE_LARGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnZoomedSnipers : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( pTFVictim->IsPlayerClass(TF_CLASS_SNIPER) && pTFVictim->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnZoomedSnipers, ACHIEVEMENT_TF_PYRO_BURN_SNIPERS_ZOOMED, "TF_PYRO_BURN_SNIPERS_ZOOMED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnChargedMedics : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( pTFVictim->IsPlayerClass(TF_CLASS_MEDIC) && pTFVictim->MedicGetChargeLevel() >= 1.0 )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnChargedMedics, ACHIEVEMENT_TF_PYRO_BURN_MEDICS_CHARGED, "TF_PYRO_BURN_MEDICS_CHARGED", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillHeaviesWithFlamethrower : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_HEAVYWEAPONS) && event->GetInt( "weaponid" ) == TF_WEAPON_FLAMETHROWER )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillHeaviesWithFlamethrower, ACHIEVEMENT_TF_PYRO_KILL_HEAVIES, "TF_PYRO_KILL_HEAVIES", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillUnderwater : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			if ( pLocalPlayer->GetWaterLevel() >= WL_Eyes && pVictim->GetWaterLevel() >= WL_Waist )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillUnderwater, ACHIEVEMENT_TF_PYRO_KILL_UNDERWATER, "TF_PYRO_KILL_UNDERWATER", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_AssistMedic : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
		m_iPlayersKilled = 0;
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
				m_iPlayersKilled = 0;
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			if ( pLocalPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pLocalPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED ) ||
				 pLocalPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
			{
				m_iPlayersKilled++;
				if ( m_iPlayersKilled >= 3 )
				{
					IncrementCount();
				}
			}
		}
	}

private:
	int		m_iPlayersKilled;
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_AssistMedic, ACHIEVEMENT_TF_PYRO_KILL_UBERCHARGE, "TF_PYRO_KILL_UBERCHARGE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_DestroyBuildings : public CBaseTFAchievement
{
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
		if ( Q_strcmp( event->GetName(), "object_destroyed" ) == 0 )
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
DECLARE_ACHIEVEMENT( CAchievementTFPyro_DestroyBuildings, ACHIEVEMENT_TF_PYRO_DESTROY_BUILDINGS, "TF_PYRO_DESTROY_BUILDINGS", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 500 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillGrind, ACHIEVEMENT_TF_PYRO_KILL_GRIND, "TF_PYRO_KILL_GRIND", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillGrindLarge : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1000 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillGrindLarge, ACHIEVEMENT_TF_PYRO_KILL_GRIND_LARGE, "TF_PYRO_KILL_GRIND_LARGE", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillFromBehindWithFlamethrower : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// Kill 50 players from behind with the flamethrower
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillFromBehindWithFlamethrower, ACHIEVEMENT_TF_PYRO_KILL_FROM_BEHIND, "TF_PYRO_KILL_FROM_BEHIND", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnTauntingSpy : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		// This will 'fail' if we add new PDAs that have a different taunt
		if ( pTFVictim->IsPlayerClass(TF_CLASS_SPY) && pTFVictim->IsTaunting() && 
			 pTFVictim->GetActiveTFWeapon() && pTFVictim->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_PDA_SPY )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnTauntingSpy, ACHIEVEMENT_TF_PYRO_BURN_SPY_TAUNT, "TF_PYRO_BURN_SPY_TAUNT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillTaunters : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && pVictim && pVictim->IsPlayer() && pVictim != pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && pTFVictim->IsTaunting() )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillTaunters, ACHIEVEMENT_TF_PYRO_KILL_TAUNTERS, "TF_PYRO_KILL_TAUNTERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_DoubleKO : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		// Clear data on level init
		ResetDeaths();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// Retire expired deaths
		int iCount = m_DeathHistory.Count();
		for ( int i = iCount-1; i >= 0; i-- )
		{
			if ( ( gpGlobals->curtime - m_DeathHistory[i].m_flTime ) > 1.0f )
			{
				m_DeathHistory.Remove( i );
			}
		}

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pVictim == pLocalPlayer )
		{
			m_flLocalDeathTime = gpGlobals->curtime;
			m_hLocalKiller = pAttacker;
			EvaluateDeaths();
		}
		else if ( pAttacker == pLocalPlayer && pVictim->IsPlayer() )
		{
			int index = m_DeathHistory.AddToTail();
			m_DeathHistory[index].m_hTarget = pVictim;
			m_DeathHistory[index].m_flTime = gpGlobals->curtime;
			EvaluateDeaths();
		}
	}

	void EvaluateDeaths( void )
	{
		if ( m_flLocalDeathTime == -1 )
			return;

		// See if we've died within 1 second of a victim's death
		int iCount = m_DeathHistory.Count();
		for ( int i = 0; i < iCount; i++ )
		{
			if ( fabs(m_flLocalDeathTime - m_DeathHistory[i].m_flTime) <= 1.0 )
			{
				if ( m_hLocalKiller == m_DeathHistory[i].m_hTarget )
				{
					IncrementCount();
					return;
				}
			}
		}
	}

	void ResetDeaths( void )
	{
		m_flLocalDeathTime = -1;
		m_hLocalKiller = NULL;
		m_DeathHistory.Purge();
	}

private:
	typedef struct
	{
		EHANDLE m_hTarget;
		float	m_flTime;
	} target_history_t;
	CUtlVector< target_history_t >	m_DeathHistory;
	float							m_flLocalDeathTime;
	EHANDLE							m_hLocalKiller;
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_DoubleKO, ACHIEVEMENT_TF_PYRO_DOUBLE_KO, "TF_PYRO_DOUBLE_KO", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_ForceEnemiesIntoWater : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}


	// Force 20 burning enemies into water.
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_ForceEnemiesIntoWater, ACHIEVEMENT_TF_PYRO_FORCE_WATERJUMP, "TF_PYRO_FORCE_WATERJUMP", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_MultiWeaponKills : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	// Kill 20 enemies that you've ignited with one of your other weapons.
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_MultiWeaponKills, ACHIEVEMENT_TF_PYRO_KILL_MULTIWEAPONS, "TF_PYRO_KILL_MULTIWEAPONS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_RageQuit : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Cause a dominated player to leave the server
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_RageQuit, ACHIEVEMENT_TF_PYRO_DOMINATE_LEAVESVR, "TF_PYRO_DOMINATE_LEAVESVR", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillWithTaunt : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim && ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_HADOUKEN || event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON || event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TAUNTATK_GASBLAST ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillWithTaunt, ACHIEVEMENT_TF_PYRO_KILL_TAUNT, "TF_PYRO_KILL_TAUNT", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnSimulScouts : public CAchievementTFPyro_BaseBurn
{
	DECLARE_CLASS( CAchievementTFPyro_BurnSimulScouts, CAchievementTFPyro_BaseBurn );
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		BaseClass::ListenForEvents();

		// Clear data on level init
		m_hBurnedScouts.Purge();
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( !pTFVictim->IsPlayerClass(TF_CLASS_SCOUT ))
			return;

		if ( m_hBurnedScouts.Find(pTFVictim) != m_hBurnedScouts.InvalidIndex() )
			return;

		// Remove any non-burning scouts from the list
		int iCount = m_hBurnedScouts.Count();
		for ( int i = iCount-1; i >= 0; i-- )
		{
			if ( !m_hBurnedScouts[i] || !m_hBurnedScouts[i]->m_Shared.InCond(TF_COND_BURNING) )
			{
				m_hBurnedScouts.Remove(i);
			}
		}

		m_hBurnedScouts.AddToTail( pTFVictim );

		if ( m_hBurnedScouts.Count() >= 2 )
		{
			IncrementCount(); 
		}
	}

private:
	CUtlVector< CHandle<C_TFPlayer> >	m_hBurnedScouts;
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnSimulScouts, ACHIEVEMENT_TF_PYRO_SIMULBURN_SCOUTS, "TF_PYRO_SIMULBURN_SCOUTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnSappingSpies : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	// Ignite a spy who's got a sapper on a friendly sentrygun
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnSappingSpies, ACHIEVEMENT_TF_PYRO_KILL_SPIES, "TF_PYRO_KILL_SPIES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnTeleportees : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( pTFVictim->m_Shared.InCond(TF_COND_TELEPORTED) )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnTeleportees, ACHIEVEMENT_TF_PYRO_CAMP_TELEPORTERS, "TF_PYRO_CAMP_TELEPORTERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillCamping : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		m_iKills = -1;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && pVictim != pLocalPlayer )
		{
			// If it's too far from our previous position, start again
			Vector vecOrg = pVictim->GetAbsOrigin();
			if ( m_iKills == -1 || ((m_vecPosition - vecOrg).LengthSqr() > (1024*1024)) )
			{
				m_vecPosition = vecOrg;
				m_iKills = 1;
			}
			else
			{
				m_iKills++;
			}

			if ( m_iKills >= 3 )
			{
				IncrementCount();
			}
		}
	}

private:
	Vector	m_vecPosition;
	int		m_iKills;
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillCamping, ACHIEVEMENT_TF_PYRO_CAMP_POSITION, "TF_PYRO_CAMP_POSITION", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_DefendControlPoints : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		SetStoreProgressInSteam( true );
	}

	// Burn a player who's capping a control point
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_DefendControlPoints, ACHIEVEMENT_TF_PYRO_DEFEND_POINTS, "TF_PYRO_DEFEND_POINTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnRJer : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Burn a rocket jumping soldier in mid-air
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnRJer, ACHIEVEMENT_TF_PYRO_BURN_RJ_SOLDIER, "TF_PYRO_BURN_RJ_SOLDIER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_FreezeTaunts : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	// Give opponents freezecams of you taunting
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_FreezeTaunts, ACHIEVEMENT_TF_PYRO_FREEZECAM_TAUNTS, "TF_PYRO_FREEZECAM_TAUNTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_BurnMedicPair : public CAchievementTFPyro_BaseBurn
{
	DECLARE_CLASS( CAchievementTFPyro_BurnMedicPair, CAchievementTFPyro_BaseBurn );
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents( void )
	{
		BaseClass::ListenForEvents();

		ListenForGameEvent( "localplayer_respawn" );

		// Clear data on level init
		m_hBurnVictims.Purge();
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		bool bIgnitedPair = false;
		// If our victim is a Medic, and we've ignited his target already, we're done.
		if ( pTFVictim->IsPlayerClass(TF_CLASS_MEDIC) )
		{
			CBaseEntity	*pTarget = pTFVictim->MedicGetHealTarget();
			if ( pTarget )
			{
				C_TFPlayer *pPlayer = ToTFPlayer(pTarget);
				if ( pPlayer && m_hBurnVictims.Find(pPlayer) != m_hBurnVictims.InvalidIndex() )
				{
					// We've ignited the medic target previously. If he's still burning, we're done.
					bIgnitedPair = (pPlayer->m_Shared.InCond(TF_COND_BURNING));
				}
			}
		}

		// See if any of our previous targets are medics, healing this new target
		int iCount = m_hBurnVictims.Count();
		for ( int i = iCount-1; i >= 0; i-- )
		{
			// Remove players from the list when they're not burning anymore
			if ( !m_hBurnVictims[i] || !m_hBurnVictims[i]->m_Shared.InCond(TF_COND_BURNING) )
			{
				m_hBurnVictims.Remove(i);
			}
			else
			{
				CBaseEntity	*pTarget = m_hBurnVictims[i]->MedicGetHealTarget();
				if ( pTarget == pTFVictim )
				{
					bIgnitedPair = true;
				}
			}
		}

		if ( bIgnitedPair )
		{
			IncrementCount(); 
			m_hBurnVictims.Purge();
		}
		else
		{
			if ( m_hBurnVictims.Find(pTFVictim) == m_hBurnVictims.InvalidIndex() )
			{
				m_hBurnVictims.AddToTail( pTFVictim );
			}
		}
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		const char *pszEventName = event->GetName();

		if ( FStrEq( pszEventName, "localplayer_respawn" ) )
		{
			m_hBurnVictims.Purge();
		}

		BaseClass::FireGameEvent_Internal(event);
	}

private:
	CUtlVector< CHandle<C_TFPlayer> > m_hBurnVictims;
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_BurnMedicPair, ACHIEVEMENT_TF_PYRO_BURN_MEDICPAIR, "TF_PYRO_BURN_MEDICPAIR", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_KillTeamwork : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// Kill an enemy who was ignited by another Pyro
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_KillTeamwork, ACHIEVEMENT_TF_PYRO_KILL_TEAMWORK, "TF_PYRO_KILL_TEAMWORK", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_IgniteFlaregun : public CAchievementTFPyro_BaseBurn
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
		SetStoreProgressInSteam( true );
	}

	void BurnedVictim( CBaseEntity *pPyro, C_TFPlayer *pTFVictim, IGameEvent *event )
	{
		if ( event->GetInt( "weaponid" ) == TF_WEAPON_FLAREGUN )
		{
			IncrementCount(); 
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_IgniteFlaregun, ACHIEVEMENT_TF_PYRO_IGNITE_FLAREGUN, "TF_PYRO_IGNITE_FLAREGUN", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_ReflectProjectiles : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 100 );
	}

	// Reflect projectiles with the compression blast
	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_ReflectProjectiles, ACHIEVEMENT_TF_PYRO_REFLECT_PROJECTILES, "TF_PYRO_REFLECT_PROJECTILES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_ReflectCrocketKill : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pAttacker == pLocalPlayer && (event->GetInt( "weaponid" ) == TF_WEAPON_ROCKETLAUNCHER) && (event->GetInt("damagebits") & DMG_CRITICAL) )
		{
			C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( pTFVictim->IsPlayerClass(TF_CLASS_SOLDIER) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_ReflectCrocketKill, ACHIEVEMENT_TF_PYRO_REFLECT_CROCKET_KILL, "TF_PYRO_REFLECT_CROCKET_KILL", 5 );

class CAchievementTFPyro_DamageGrind : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1000000 );		
	}

	void OnPlayerStatsUpdate()
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( TF_CLASS_PYRO );
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[TFSTAT_FIREDAMAGE];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_PYRO ) )
		{
			EvaluateNewAchievement();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_DamageGrind, ACHIEVEMENT_TF_PYRO_DAMAGE_GRIND, "TF_PYRO_DAMAGE_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_AchieveProgress1 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFPyro_AchieveProgress1, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 10, ACHIEVEMENT_TF_PYRO_START_RANGE, ACHIEVEMENT_TF_PYRO_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_AchieveProgress1, ACHIEVEMENT_TF_PYRO_ACHIEVE_PROGRESS1, "TF_PYRO_ACHIEVE_PROGRESS1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_AchieveProgress2 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFPyro_AchieveProgress2, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 16, ACHIEVEMENT_TF_PYRO_START_RANGE, ACHIEVEMENT_TF_PYRO_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_AchieveProgress2, ACHIEVEMENT_TF_PYRO_ACHIEVE_PROGRESS2, "TF_PYRO_ACHIEVE_PROGRESS2", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_AchieveProgress3 : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFPyro_AchieveProgress3, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 22, ACHIEVEMENT_TF_PYRO_START_RANGE, ACHIEVEMENT_TF_PYRO_END_RANGE );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_AchieveProgress3, ACHIEVEMENT_TF_PYRO_ACHIEVE_PROGRESS3, "TF_PYRO_ACHIEVE_PROGRESS3", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_IgniteWithRainbow : public CBaseTFAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_IgniteWithRainbow, ACHIEVEMENT_TF_PYRO_IGNITE_WITH_RAINBOW, "TF_PYRO_IGNITE_WITH_RAINBOW", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFPyro_IgnitePlayerBeingFlipped : public CBaseTFAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFPyro_IgnitePlayerBeingFlipped, ACHIEVEMENT_TF_PYRO_IGNITE_PLAYER_BEING_FLIPPED, "TF_PYRO_IGNITE_PLAYER_BEING_FLIPPED", 5 );

#endif // CLIENT_DLL



