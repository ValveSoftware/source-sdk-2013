//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: static_prop - don't move, don't animate, don't do anything.
//			physics_prop - move, take damage, but don't animate
//
//===========================================================================//

#ifndef TF_PROPS_H
#define TF_PROPS_H

#include "props.h"
#include "triggers.h"
#include "tf_player.h"

class CPropSoccerBall : public CPhysicsProp
{
	DECLARE_CLASS( CPropSoccerBall, CPhysicsProp );
public:
	CPropSoccerBall()
		: m_flNextAllowedImpactTime( 0.f ), m_hLastToucher( NULL )
	{}

	DECLARE_DATADESC()

	virtual void Precache();
	virtual void Spawn();

	// Here's the deal.  The ball is a trigger, but triggers are not allowed to touch other triggers.  To get around this,
	// we're going to specify the names of the triggers we actually want to touch and then we're going to manually try to 
	// touch them.  Our collision system is a vortex of insanity.
	void TriggerTouchThink();
	virtual void Activate() OVERRIDE;
	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent ){}
	void BallTouch( CBaseEntity *pOther );
	CTFPlayer *GetLastToucher( void ){ return m_hLastToucher.Get(); }

	virtual bool ShouldBlockNav() const OVERRIDE { return false; }

private:
	string_t m_iszTriggers;
	float m_flNextAllowedImpactTime;
	CUtlVector< CBaseTrigger* > m_vecTriggers;
	CHandle< CTFPlayer > m_hLastToucher;
};

#endif // TF_PROPS_H
