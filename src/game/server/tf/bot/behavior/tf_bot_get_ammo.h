//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_get_ammo.h
// Pick up any nearby ammo
// Michael Booth, May 2009

#ifndef TF_BOT_GET_AMMO_H
#define TF_BOT_GET_AMMO_H

#include "tf_powerup.h"

class CTFBotGetAmmo : public Action< CTFBot >
{
public:
	CTFBotGetAmmo( void );

	static bool IsPossible( CTFBot *me );			// return true if this Action has what it needs to perform right now

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "GetAmmo"; };

private:
	PathFollower m_path;
	CHandle< CBaseEntity > m_ammo;
	bool m_isGoalDispenser;
};


#endif // TF_BOT_GET_AMMO_H
