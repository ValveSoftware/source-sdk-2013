//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef GAME_DLL

// this gets compiled in for HL2 + Ep(X) only
#if ( defined( HL2_DLL ) || defined( HL2_EPISODIC ) ) && ( !defined ( PORTAL ) )

#include "baseachievement.h"
#include "prop_combine_ball.h"
#include "combine_mine.h"
#include "basegrenade_shared.h"
#include "basehlcombatweapon_shared.h"
#include "ammodef.h"

class CAchievementHLXKillWithPhysicsObjects : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "prop_physics" );
		SetGoal( 30 );
		
		if ( IsPC() )
		{
			// only in Ep2 for PC. (Shared across HLX for X360.)
			SetGameDirFilter( "ep2" );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		int iDamageBits = event->GetInt( "damagebits" );
		// was victim killed with crushing damage?
		if ( iDamageBits & DMG_CRUSH ) 
		{
			IncrementCount();
		}		
	}

};
DECLARE_ACHIEVEMENT( CAchievementHLXKillWithPhysicsObjects, ACHIEVEMENT_HLX_KILL_ENEMIES_WITHPHYSICS, "HLX_KILL_ENEMIES_WITHPHYSICS", 5 );

class CAchievementHLXKillWithHopper : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetAttackerFilter( "combine_mine" );
		SetGoal( 1 );

		if ( IsPC() )
		{
			// only in Ep2 for PC. (Shared across HLX for X360.)
			SetGameDirFilter( "ep2" );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// If we get here, a combine mine has killed a player enemy.  Now check and see if the player planted it
		CBounceBomb *pBounceBomb = dynamic_cast<CBounceBomb *>( pAttacker );
		if ( pBounceBomb && pBounceBomb->IsPlayerPlaced() )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementHLXKillWithHopper, ACHIEVEMENT_HLX_KILL_ENEMY_WITHHOPPERMINE, "HLX_KILL_ENEMY_WITHHOPPERMINE", 5 );

class CAchievementHLXKillWithManhack : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "npc_manhack" );
		SetGoal( 5 );

		if ( IsPC() )
		{
			// only in HL2 for PC. (Shared across HLX for X360.)
			SetGameDirFilter( "hl2" );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// We've already filtered to only get called when a player enemy gets killed with a manhack.  Now just check for the
		// case of player smashing manhack into something, in which case the manhack is both the victim and inflictor.
		// If that's not the case, this is a player kill w/manhack.
		if ( pVictim != pInflictor )	
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementHLXKillWithManhack, ACHIEVEMENT_HLX_KILL_ENEMIES_WITHMANHACK, "HLX_KILL_ENEMIES_WITHMANHACK", 5 );

class CAchievementHLXKillSoldierWithOwnGrenade : public CBaseAchievement
{
protected:
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "npc_grenade_frag" );
		SetVictimFilter( "npc_combine_s" );
		SetGoal( 1 );

		if ( IsPC() )
		{
			// only in Ep2 for PC. (Shared across HLX for X360.)
			SetGameDirFilter( "ep2" );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>( pInflictor );
		if ( pGrenade )
		{
			CBaseEntity *pThrower = pGrenade->GetThrower();
			CBaseEntity *pOriginalThrower = pGrenade->GetOriginalThrower();
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			// check if player was most recent thrower, but the victim was the original thrower
			if ( ( pPlayer == pThrower ) && ( pOriginalThrower == pVictim ) )
			{
				IncrementCount();
			}				
		}			
	}
};
DECLARE_ACHIEVEMENT( CAchievementHLXKillSoldierWithOwnGrenade, ACHIEVEMENT_HLX_KILL_SOLDIER_WITHHISGRENADE, "HLX_KILL_SOLDIER_WITHHISGRENADE", 10 );

class CAchievementHLXKillWithOneEnergyBall : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "prop_combine_ball" );
		SetGoal( 1 );
		m_pLastInflictor = NULL;
		m_iLocalCount = 0;

		if ( IsPC() )
		{
			// only in Ep1 for PC. (Shared across HLX for X360.)
			SetGameDirFilter( "episodic" );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// to count # of kills with same energy ball, keep track of previous inflictor
		if ( m_pLastInflictor != NULL && pInflictor != m_pLastInflictor )
		{
			// new inflictor, start the count over at 1
			m_iLocalCount = 1;
		}
		else
		{
			// same inflictor, keep counting
			m_iLocalCount++;
			if ( 5 == m_iLocalCount )
			{
				IncrementCount();
			}
		}
		// keep track of last inflictor
		m_pLastInflictor = pInflictor;
	}
	CBaseEntity *m_pLastInflictor;
	int m_iLocalCount;
};
DECLARE_ACHIEVEMENT( CAchievementHLXKillWithOneEnergyBall, ACHIEVEMENT_HLX_KILL_ENEMIES_WITHONEENERGYBALL, "HLX_KILL_ENEMIES_WITHONEENERGYBALL", 5 );

class CAchievementHLXKillEliteSoldierWithOwnEnergyBall : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "prop_combine_ball" );
		SetVictimFilter( "npc_combine_s" );
		SetGoal( 1 );

		if ( IsPC() )
		{
			// only in Ep2 for PC. (Shared across HLX for X360.)
			SetGameDirFilter( "episodic" );
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CPropCombineBall *pBall = dynamic_cast<CPropCombineBall *>( pInflictor );
		if ( pBall )
		{
			// determine original owner of this ball
			CBaseEntity *pOriginalOwner = pBall->GetOriginalOwner();
			// see if original owner is the victim
			if ( pOriginalOwner && ( pOriginalOwner == pVictim ) )
			{				
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementHLXKillEliteSoldierWithOwnEnergyBall, ACHIEVEMENT_HLX_KILL_ELITESOLDIER_WITHHISENERGYBALL, "HLX_KILL_ELITESOLDIER_WITHHISENERGYBALL", 10 );

//-----------------------------------------------------------------------------
// Purpose: Counts the accumulated # of primary and secondary attacks from all
//			weapons (except grav gun).  If bBulletOnly is true, only counts
//			attacks with ammo that does bullet damage.
//-----------------------------------------------------------------------------
int CalcPlayerAttacks( bool bBulletOnly )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	CAmmoDef *pAmmoDef = GetAmmoDef();
	if ( !pPlayer || !pAmmoDef )
		return 0;

	int iTotalAttacks = 0;
	int iWeapons = pPlayer->WeaponCount();
	for ( int i = 0; i < iWeapons; i++ )
	{
		CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>( pPlayer->GetWeapon( i ) );
		if ( pWeapon )
		{
			// add primary attacks if we were asked for all attacks, or only if it uses bullet ammo if we were asked to count bullet attacks
			if ( !bBulletOnly || ( pAmmoDef->m_AmmoType[pWeapon->GetPrimaryAmmoType()].nDamageType == DMG_BULLET ) )
			{
				iTotalAttacks += pWeapon->m_iPrimaryAttacks;
			}
			// add secondary attacks if we were asked for all attacks, or only if it uses bullet ammo if we were asked to count bullet attacks
			if ( !bBulletOnly || ( pAmmoDef->m_AmmoType[pWeapon->GetSecondaryAmmoType()].nDamageType == DMG_BULLET ) )
			{
				iTotalAttacks += pWeapon->m_iSecondaryAttacks;
			}
		}
	}
	return iTotalAttacks;
}

#endif	// ( defined( HL2_DLL ) || defined( HL2_EPISODIC ) ) && ( !defined ( PORTAL ) )

#endif // GAME_DLL
