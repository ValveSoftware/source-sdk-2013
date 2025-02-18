//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "../headless_hatman_body.h"
#include "Path/NextBotPathFollow.h"

class CZombie;


//----------------------------------------------------------------------------
class CZombieLocomotion : public NextBotGroundLocomotion
{
public:
	CZombieLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CZombieLocomotion() { }

	virtual float GetRunSpeed( void ) const;			// get maximum running speed
	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump

	virtual bool ShouldCollideWith( const CBaseEntity *object ) const;

private:
	virtual float GetMaxYawRate( void ) const;				// return max rate of yaw rotation
};


//----------------------------------------------------------------------------
class CZombieIntention : public IIntention
{
public:
	CZombieIntention( CZombie *me );
	virtual ~CZombieIntention();

	virtual void Reset( void );
	virtual void Update( void );

	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const;	// is the a place we can be?

	virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return m_behavior; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }

private:
	Behavior< CZombie > *m_behavior;
};


//----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IZombieAutoList );

class CZombie : public NextBotCombatCharacter, public IZombieAutoList
{
public:
	DECLARE_CLASS( CZombie, NextBotCombatCharacter );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CZombie();
	virtual ~CZombie();

	static void PrecacheZombie();
	virtual void Precache();
	virtual void Spawn( void );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void UpdateOnRemove();

	// INextBot
	virtual CZombieIntention	*GetIntentionInterface( void ) const	{ return m_intention; }
	virtual CZombieLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CHeadlessHatmanBody	*GetBodyInterface( void ) const			{ return m_body; }

	CBaseAnimating *m_zombieParts;

	void StartLifeTimer( float flLifeTime ) { m_lifeTimer.Start( flLifeTime ); }
	bool ShouldSuicide() const;
	void ForceSuicide() { m_bForceSuicide = true; }

	enum SkeletonType_t
	{
		SKELETON_NORMAL = 0,
		SKELETON_KING,
		SKELETON_MINI
	};
	SkeletonType_t GetSkeletonType() const { return m_nType; }
	void SetSkeletonType( SkeletonType_t nType );
	void AddHat( const char *pszModel );

	static CZombie* SpawnAtPos( const Vector& vSpawnPos, float flLifeTime = 0.f, int nTeam = TF_TEAM_HALLOWEEN, CBaseEntity *pOwner = NULL, SkeletonType_t nSkeletonType = SKELETON_NORMAL );

	float GetAttackRange() const { return m_flAttackRange; }
	float GetAttackDamage() const { return m_flAttackDamage; }

	void FireDeathOutput( CBaseEntity *pCulprit );
private:
	CZombieIntention *m_intention;
	CZombieLocomotion *m_locomotor;
	CHeadlessHatmanBody *m_body;

	SkeletonType_t m_nType;
	CNetworkVar( float, m_flHeadScale );

	CHandle< CBaseAnimating > m_hHat;

	struct RecentDamager_t
	{
		EHANDLE m_hEnt;
		float m_flDamageTime;
	};

	CUtlVector< RecentDamager_t > m_vecRecentDamagers;

	float m_flAttackRange;
	float m_flAttackDamage;

	bool m_bSpy;
	bool m_bForceSuicide;
	CountdownTimer m_lifeTimer;

	bool m_bDeathOutputFired;
	COutputEvent m_OnDeath;
};



//--------------------------------------------------------------------------------------------------------------
class CZombiePathCost : public IPathCost
{
public:
	CZombiePathCost( CZombie *me )
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
				dist += jumpPenalty * dist;
			}
			else if ( deltaZ < -m_me->GetLocomotionInterface()->GetDeathDropHeight() )
			{
				// too far to drop
				return -1.0f;
			}

			// this term causes the same bot to choose different routes over time,
			// but keep the same route for a period in case of repaths
			int timeMod = (int)( gpGlobals->curtime / 10.0f ) + 1;
			float preference = 1.0f + 50.0f * ( 1.0f + FastCos( (float)( m_me->GetEntity()->entindex() * area->GetID() * timeMod ) ) );
			float cost = dist * preference;

			return cost + fromArea->GetCostSoFar();
		}
	}

	CZombie *m_me;
};


#endif // ZOMBIE_H
