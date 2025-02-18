//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_ATTACK_H
#define HL2MP_BOT_ATTACK_H

#include "Path/NextBotChasePath.h"


//-------------------------------------------------------------------------------
class CHL2MPBotAttack : public Action< CHL2MPBot >
{
public:
	CHL2MPBotAttack( void );
	virtual ~CHL2MPBotAttack() { }

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot *me );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToSuccess( CHL2MPBot *me, const Path *path );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "Attack"; };

private:
	PathFollower m_path;
	ChasePath m_chasePath;
	CountdownTimer m_repathTimer;
};


#endif // HL2MP_BOT_ATTACK_H
