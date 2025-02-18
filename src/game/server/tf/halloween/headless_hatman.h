//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman.h
// An NPC that spawns in the Halloween map and wreaks havok
// Michael Booth, October 2010

#ifndef HEADLESS_HATMAN_H
#define HEADLESS_HATMAN_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "headless_hatman_body.h"
#include "Path/NextBotPathFollow.h"
#include "halloween_base_boss.h"

extern ConVar tf_halloween_bot_health_base;
extern ConVar tf_halloween_bot_health_per_player;
extern ConVar tf_halloween_bot_min_player_count;

extern ConVar tf_halloween_bot_speed;
extern ConVar tf_halloween_bot_attack_range;
extern ConVar tf_halloween_bot_speed_recovery_rate;
extern ConVar tf_halloween_bot_speed_penalty;
extern ConVar tf_halloween_bot_chase_duration;
extern ConVar tf_halloween_bot_terrify_radius;
extern ConVar tf_halloween_bot_chase_range;
extern ConVar tf_halloween_bot_quit_range;

class CTFPlayer;
class CHeadlessHatman;


//----------------------------------------------------------------------------
class CHeadlessHatmanLocomotion : public NextBotGroundLocomotion
{
public:
	CHeadlessHatmanLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CHeadlessHatmanLocomotion() { }

	virtual float GetRunSpeed( void ) const;			// get maximum running speed
	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump

	/**
	 * Should we collide with this entity?
	 */
	virtual bool ShouldCollideWith( const CBaseEntity *object ) const;

private:
	virtual float GetMaxYawRate( void ) const;				// return max rate of yaw rotation
};


//----------------------------------------------------------------------------
class CHeadlessHatmanIntention : public IIntention
{
public:
	CHeadlessHatmanIntention( CHeadlessHatman *me );
	virtual ~CHeadlessHatmanIntention();

	virtual void Reset( void );
	virtual void Update( void );

	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const;	// is the a place we can be?

	virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return m_behavior; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }

private:
	Behavior< CHeadlessHatman > *m_behavior;
};


//----------------------------------------------------------------------------
class CHeadlessHatman : public CHalloweenBaseBoss
{
public:
	DECLARE_CLASS( CHeadlessHatman, CHalloweenBaseBoss );
	DECLARE_SERVERCLASS();

	CHeadlessHatman();
	virtual ~CHeadlessHatman();

	static void PrecacheHeadlessHatman();
	virtual void Precache();
	virtual void Spawn( void );

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	// INextBot
	virtual CHeadlessHatmanIntention *GetIntentionInterface( void ) const		{ return m_intention; }
	virtual CHeadlessHatmanLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CHeadlessHatmanBody *GetBodyInterface( void ) const					{ return m_body; }

	virtual void Update( void );

	const Vector &GetHomePosition( void ) const;

	CBaseAnimating *GetAxe( void ) const;

	virtual HalloweenBossType GetBossType() const { return HALLOWEEN_BOSS_HHH; }

private:
	const char *GetWeaponModel() const;

	CHeadlessHatmanIntention *m_intention;
	CHeadlessHatmanLocomotion *m_locomotor;
	CHeadlessHatmanBody *m_body;

	CBaseAnimating *m_axe;

	CUtlVector< AttackerInfo > m_attackerVector;		// list of everyone who injured me, and when

	CountdownTimer m_painTimer;

	Vector m_homePos;
	int m_damagePoseParameter;
};


inline CBaseAnimating *CHeadlessHatman::GetAxe( void ) const
{
	return m_axe;
}


inline const Vector &CHeadlessHatman::GetHomePosition( void ) const
{
	return m_homePos;
}


//--------------------------------------------------------------------------------------------------------------
class CHeadlessHatmanPathCost : public IPathCost
{
public:
	CHeadlessHatmanPathCost( CHeadlessHatman *me )
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

	CHeadlessHatman *m_me;
};


#endif // HEADLESS_HATMAN_H
