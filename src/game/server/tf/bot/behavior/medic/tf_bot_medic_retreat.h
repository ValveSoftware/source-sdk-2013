//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_retreat.cpp
// Retreat towards our spawn to find another patient
// Michael Booth, May 2009

#ifndef TF_BOT_MEDIC_RETREAT_H
#define TF_BOT_MEDIC_RETREAT_H

#include "Path/NextBotChasePath.h"

class CTFBotMedicRetreat : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;

	virtual const char *GetName( void ) const	{ return "Retreat"; };

private:
	PathFollower m_path;
	CountdownTimer m_lookAroundTimer;
};

#endif // TF_BOT_MEDIC_RETREAT_H
