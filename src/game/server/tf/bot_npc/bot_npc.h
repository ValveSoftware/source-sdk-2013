//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc.h
// A NextBot non-player derived actor
// Michael Booth, November 2010

#ifndef BOT_NPC_H
#define BOT_NPC_H

#ifdef OBSOLETE_USE_BOSS_ALPHA

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"
#include "bot_npc_body.h"
#include "bot/map_entities/tf_spawner_boss.h"

class CTFPlayer;
class CBotNPC;


//----------------------------------------------------------------------------
class CBotNPCLocomotion : public NextBotGroundLocomotion
{
public:
	CBotNPCLocomotion( INextBot *bot );
	virtual ~CBotNPCLocomotion() { }

	virtual float GetRunSpeed( void ) const;				// get maximum running speed
	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump

	virtual float GetMaxAcceleration( void ) const
	{
		return 2500.0f;
	}

private:
	float m_runSpeed;
};


//----------------------------------------------------------------------------
class CBotNPCIntention : public IIntention
{
public:
	CBotNPCIntention( CBotNPC *me );
	virtual ~CBotNPCIntention();

	virtual void Reset( void );
	virtual void Update( void );

	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const;	// is the a place we can be?

	virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return m_behavior; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }

private:
	Behavior< CBotNPC > *m_behavior;
};


//----------------------------------------------------------------------------
class CBotNPCVision : public IVision
{
public:
	CBotNPCVision( INextBot *bot ) : IVision( bot )
	{
	}

	virtual ~CBotNPCVision() { }

	virtual bool IsIgnored( CBaseEntity *subject ) const;		// return true to completely ignore this entity (may not be in sight when this is called)
};


//----------------------------------------------------------------------------
class CBotNPCWeapon : public CBaseAnimating
{
public:
	CBotNPCWeapon( CBotNPC *owner )
	{
		m_owner = owner;
	}

	virtual ~CBotNPCWeapon() { }

	virtual void StartAttack( void )	{ }
	virtual void Update( void )			{ }

private:
	CHandle< CBotNPC > m_owner;
};


//----------------------------------------------------------------------------
class CBotNPCWeapon_Axe : public CBotNPCWeapon
{
public:
	DECLARE_CLASS( CBotNPCWeapon_Axe, CBotNPCWeapon );

	CBotNPCWeapon_Axe( CBotNPC *owner );
	virtual ~CBotNPCWeapon_Axe() { }

	virtual void StartAttack( void );
	virtual void Update( void );
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class CBotNPCGetOffMe : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual const char *GetName( void ) const	{ return "GetOffMe"; }		// return name of this action

private:
	CountdownTimer m_timer;
};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class CBotNPC : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CBotNPC, NextBotCombatCharacter );
	DECLARE_SERVERCLASS();

	CBotNPC();
	virtual ~CBotNPC();

	virtual void Precache();
	virtual void Spawn( void );

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	// INextBot
	virtual CBotNPCIntention *GetIntentionInterface( void ) const		{ return m_intention; }
	virtual CBotNPCLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CBotNPCBody *GetBodyInterface( void ) const					{ return m_body; }
	virtual CBotNPCVision *GetVisionInterface( void ) const				{ return m_vision; }

	virtual void Update( void );

	virtual bool IsPotentiallyChaseable( CTFPlayer *victim );

	void SetSpawner( CTFSpawnerBoss *spawner );				// remember the spawner that created us
	CTFSpawnerBoss *GetSpawner( void ) const;				// return the spawner that created us

	void Break( void );		// bust into gibs

	struct AttackerInfo
	{
		CHandle< CBaseCombatCharacter > m_attacker;
		float m_timestamp;
		float m_damage;
		bool m_wasCritical;
	};
	const CUtlVector< AttackerInfo > &GetAttackerVector( void ) const;
	void RememberAttacker( CBaseCombatCharacter *attacker, float damage, bool wasCritical );

	struct ThreatInfo
	{
		CHandle< CBaseCombatCharacter > m_who;
		float m_threat;
	};

	const ThreatInfo *GetMaxThreat( void ) const;
	const ThreatInfo *GetThreat( CBaseCombatCharacter *who ) const;

	void SwingAxe( void );
	void UpdateAxeSwing( void );
	bool IsSwingingAxe( void ) const;

	
	//----------------------------------
	enum Ability
	{
		CAN_BE_STUNNED		= 0x01,
		CAN_NUKE			= 0x02,
		CAN_ENRAGE			= 0x04,
		CAN_FIRE_ROCKETS	= 0x08,
		CAN_LAUNCH_STICKIES	= 0x10,
		CAN_LAUNCH_MINIONS	= 0x20,
	};
	virtual bool HasAbility( Ability ability ) const;

	virtual bool IsMiniBoss( void ) const					{ return false; }

	virtual float GetMoveSpeed( void ) const				{ return 300.0f; }

	virtual int GetRocketLaunchCount( void ) const			{ return 5; }
	virtual float GetRocketDamage( void ) const				{ return 25.0f; }
	virtual float GetRocketAimError( void ) const			{ return 1.81f; }
	virtual float GetRocketInterval( void ) const			{ return 0.3f; }
	virtual const char *GetRocketSoundEffect( void ) const	{ return "Weapon_RPG.Single"; }

	virtual float GetGrenadeInterval( void ) const			{ return 10.0f; }

	virtual float GetBecomeStunnedDamage( void ) const		{ return 500.0f; }


	//----------------------------------
	enum Condition
	{
		SHIELDED = 0x01,
		CHARGING = 0x02,
		STUNNED = 0x04,
		INVULNERABLE = 0x08,
		VULNERABLE_TO_STUN = 0x10,
		BUSY = 0x20,
		ENRAGED = 0x40,
	};

	bool IsBusy( void ) const;			// returns true if we're in a condition that means we can't start another action

	void AddCondition( Condition c );
	void RemoveCondition( Condition c );
	bool IsInCondition( Condition c ) const;

	bool IsAttackTarget( CBaseCombatCharacter *target ) const;
	bool HasAttackTarget( void ) const;
	void SetAttackTarget( CBaseCombatCharacter *target, float duration = 0.0f );
	CBaseCombatCharacter *GetAttackTarget( void ) const;
	void LockAttackTarget( void );		// don't allow target to change until it is unlocked or the target is destroyed
	void UnlockAttackTarget( void );

	CBaseCombatCharacter *GetNearestVisibleEnemy( void ) const;

	void SetHomePosition( const Vector &pos );
	const Vector &GetHomePosition( void ) const;

	CBaseAnimating *GetWeapon( void ) const;
	CBaseAnimating *GetShield( void ) const;

	CountdownTimer *GetNukeTimer( void );
	CountdownTimer *GetGrenadeTimer( void );

	float GetReceivedDamagePerSecond( void ) const;
	float GetReceivedDamagePerSecondDelta( void ) const;

	void SetLaserTarget( CBaseEntity *target );
	CBaseEntity *GetLaserTarget( void ) const;

	void ClearStunDamage( void );
	void AccumulateStunDamage( float damage );
	float GetStunDamage( void ) const;

	CTFPlayer *GetClosestMinionPrisoner( void );
	bool IsPrisonerOfMinion( CBaseCombatCharacter *victim );

	void StartNukeEffect( void );
	void StopNukeEffect( void );

	float GetAge( void ) const;			// how long have we been alive

	void CollectPlayersStandingOnMe( CUtlVector< CTFPlayer * > *playerVector );

	// Entity I/O
	void InputSpawn( inputdata_t &inputdata );
	COutputEvent m_outputOnStunned;	// fired the boss becomes stunned

private:
	CBotNPCIntention *m_intention;
	CBotNPCLocomotion *m_locomotor;
	CBotNPCBody *m_body;
	CBotNPCVision *m_vision;

	CHandle< CTFSpawnerBoss > m_spawner;

	CBaseAnimating *m_axe;
	CBaseAnimating *m_shield;

	void PrecacheArmorParts( void );
	void InstallArmorParts( void );
	CUtlVector< CBaseAnimating * > m_armorPartVector;

	CountdownTimer m_axeSwingTimer;
	CountdownTimer m_attackTimer;
	CountdownTimer m_nukeTimer;
	CountdownTimer m_grenadeTimer;
	CountdownTimer m_ouchTimer;
	CountdownTimer m_hateTauntTimer;

	CNetworkHandle( CBaseEntity, m_laserTarget );
	CNetworkVar( bool, m_isNuking );

	CHandle< CBaseCombatCharacter > m_nearestVisibleEnemy;
	void UpdateNearestVisibleEnemy( void );
	CountdownTimer m_nearestVisibleEnemyTimer;

	CUtlVector< AttackerInfo > m_attackerVector;		// list of everyone who injured me, and when
	CUtlVector< ThreatInfo > m_threatVector;			// list of attackers and their current damage/second on me

	float m_currentDamagePerSecond;
	float m_lastDamagePerSecond;
	void UpdateDamagePerSecond( void );

	CHandle< CBaseCombatCharacter > m_attackTarget;
	CountdownTimer m_attackTargetTimer;
	bool m_isAttackTargetLocked;
	void UpdateAttackTarget( void );

	int m_damagePoseParameter;

	bool m_isShielded;
	Vector m_homePos;

	bool IsIgnored( CTFPlayer *player ) const;

	unsigned int m_conditionFlags;

	float m_stunDamage;

	IntervalTimer m_ageTimer;
};


inline bool CBotNPC::HasAbility( Ability ability ) const
{
	const int myAbilities = CAN_BE_STUNNED | CAN_NUKE | CAN_ENRAGE | CAN_FIRE_ROCKETS | CAN_LAUNCH_STICKIES | CAN_LAUNCH_MINIONS;

	return myAbilities & ability ? true : false;
}

inline void CBotNPC::SetSpawner( CTFSpawnerBoss *spawner )
{
	m_spawner = spawner;
}

inline CTFSpawnerBoss *CBotNPC::GetSpawner( void ) const
{
	return m_spawner;
}

inline bool CBotNPC::IsAttackTarget( CBaseCombatCharacter *target ) const
{
	if ( HasAttackTarget() )
	{
		return ( m_attackTarget == target ) ? true : false;
	}
	return false;
}

inline bool CBotNPC::HasAttackTarget( void ) const
{
	return ( m_attackTarget == NULL || !m_attackTarget->IsAlive() ) ? false : true;
}

inline void CBotNPC::LockAttackTarget( void )
{
	m_isAttackTargetLocked = HasAttackTarget();
}

inline void CBotNPC::UnlockAttackTarget( void )
{
	m_isAttackTargetLocked = false;
}

inline float CBotNPC::GetAge( void ) const
{
	return m_ageTimer.GetElapsedTime();
}

inline void CBotNPC::StartNukeEffect( void )
{
	m_isNuking = true;
}

inline void CBotNPC::StopNukeEffect( void )
{
	m_isNuking = false;
}

inline void CBotNPC::ClearStunDamage( void )
{
	m_stunDamage = 0.0f;
}

inline void CBotNPC::AccumulateStunDamage( float damage )
{
	m_stunDamage += damage;
}

inline float CBotNPC::GetStunDamage( void ) const
{
	return m_stunDamage;
}

inline void CBotNPC::SetLaserTarget( CBaseEntity *target )
{
	m_laserTarget = target;
}

inline CBaseEntity *CBotNPC::GetLaserTarget( void ) const
{
	return m_laserTarget;
}

inline float CBotNPC::GetReceivedDamagePerSecond( void ) const
{
	return m_currentDamagePerSecond;
}

inline float CBotNPC::GetReceivedDamagePerSecondDelta( void ) const
{
	return m_currentDamagePerSecond - m_lastDamagePerSecond;
}

inline CountdownTimer *CBotNPC::GetNukeTimer( void )
{
	return &m_nukeTimer;
}

inline CountdownTimer *CBotNPC::GetGrenadeTimer( void )
{
	return &m_grenadeTimer;
}

inline CBaseAnimating *CBotNPC::GetWeapon( void ) const
{
	return m_axe;
}

inline CBaseAnimating *CBotNPC::GetShield( void ) const
{
	return m_shield;
}

inline void CBotNPC::SetHomePosition( const Vector &pos )
{
	m_homePos = pos;
}

inline const Vector &CBotNPC::GetHomePosition( void ) const
{
	return m_homePos;
}

inline CBaseCombatCharacter *CBotNPC::GetNearestVisibleEnemy( void ) const
{
	return m_nearestVisibleEnemy;
}

inline void CBotNPC::AddCondition( Condition c )
{
	m_conditionFlags |= c;
}

inline bool CBotNPC::IsInCondition( Condition c ) const
{
	return ( m_conditionFlags & c ) ? true : false;
}

inline const CUtlVector< CBotNPC::AttackerInfo > &CBotNPC::GetAttackerVector( void ) const
{
	return m_attackerVector;
}


//--------------------------------------------------------------------------------------------------------------
class CBotNPCPathCost : public IPathCost
{
public:
	CBotNPCPathCost( CBotNPC *me )
	{
		m_me = me;
	}

	// return the cost (weighted distance between) of moving from "fromArea" to "area", or -1 if the move is not allowed
	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if ( !m_me->GetLocomotionInterface()->IsAreaTraversable( area ) )
			{
				// our locomotor says we can't move here
				return -1.0f;
			}

			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				// optimization to avoid recomputing length
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			float cost = dist + fromArea->GetCostSoFar();

			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );
			if ( deltaZ >= m_me->GetLocomotionInterface()->GetStepHeight() )
			{
				if ( deltaZ >= m_me->GetLocomotionInterface()->GetMaxJumpHeight() )
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 5.0f;
				cost += jumpPenalty * dist;
			}
			else if ( deltaZ < -m_me->GetLocomotionInterface()->GetDeathDropHeight() )
			{
				// too far to drop
				return -1.0f;
			}

			return cost;
		}
	}

	CBotNPC *m_me;
};


#endif // #ifdef OBSOLETE_USE_BOSS_ALPHA

#endif // BOT_NPC_H
