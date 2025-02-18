//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_AOE_ATTACK_H
#define MERASMUS_AOE_ATTACK_H

class CMerasmusAOEAttack : public Action< CMerasmus >
{
public:
	virtual ActionResult< CMerasmus > OnStart( CMerasmus *me, Action< CMerasmus > *priorAction );
	virtual ActionResult< CMerasmus > Update( CMerasmus *me, float interval );
	virtual void					  OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction );

	virtual const char *GetName( void ) const	{ return "AOE Attack!"; }		// return name of this action
private:
	enum AOEState_t
	{
		AOE_BEGIN,
		AOE_FIRING,
	};
	AOEState_t m_state;

	CountdownTimer m_aoeStartTimer;
	CountdownTimer m_launchTimer;
	CountdownTimer m_flyTimer;
	CUtlVector< CNavArea * > m_wanderAreaVector;
	CountdownTimer m_wanderTimer;
	CTFNavArea *m_wanderArea;

	// To save network perf, we don't create all bombs in a single tick. Rather, we fill up a queue of bombs and distribute the creation over time.
	// I originally had another property (start position) as part of MerasmusGrenadeCreateSpec_t, but it didn't make sense, since we want to use
	// his current position when we actually create -- not the position he was at whenever we actually filled the queue with grenades. I'm leaving
	// the struct here, rather than making m_vecGrenadesToCreate a CUtlVector< Vector >, in case we want to add anything else.
	struct MerasmusGrenadeCreateSpec_t
	{
		MerasmusGrenadeCreateSpec_t( const Vector &v ) : m_vecVelocity( v ) {}
		Vector m_vecVelocity;
	};
	CUtlVector< MerasmusGrenadeCreateSpec_t > m_vecGrenadesToCreate;

	void QueueSingleGrenadeForLaunch( const Vector &vecVelocity );	// Don't call directly - call QueueBombRingsForLaunch() or QueueBombSpokesForLaunch()
	void ClearPendingGrenades();
	void LaunchPendingGrenades( CMerasmus *me );

	void QueueBombRingsForLaunch( CMerasmus *me );
	void QueueBombSpokesForLaunch( CMerasmus *me );
};

#endif // MERASMUS_TELEPORT_AOE_ATTACK_H
