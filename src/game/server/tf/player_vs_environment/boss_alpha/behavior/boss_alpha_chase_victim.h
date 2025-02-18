//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_chase_victim.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_CHASE_VICTIM_H
#define BOSS_ALPHA_CHASE_VICTIM_H

#ifdef TF_RAID_MODE

#include "nav_mesh/tf_path_follower.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBossAlphaChaseVictim : public Action< CBossAlpha >
{
public:
	CBossAlphaChaseVictim( CBaseCombatCharacter *chaseTarget );

	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	virtual EventDesiredResult< CBossAlpha > OnStuck( CBossAlpha *me );
	virtual EventDesiredResult< CBossAlpha > OnMoveToSuccess( CBossAlpha *me, const Path *path );
	virtual EventDesiredResult< CBossAlpha > OnMoveToFailure( CBossAlpha *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "ChaseVictim"; }		// return name of this action

private:
	CTFPathFollower m_path;
	IntervalTimer m_visibleTimer;
	CHandle< CBaseCombatCharacter > m_lastTarget;

	CHandle< CBaseCombatCharacter > m_chaseTarget;
	Vector m_lastKnownTargetSpot;
};


#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_CHASE_VICTIM_H
