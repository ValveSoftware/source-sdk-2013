//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_GET_PROP_H
#define HL2MP_BOT_GET_PROP_H

#include "Path/NextBotChasePath.h"


class CHL2MPBotGetProp : public Action< CHL2MPBot >
{
public:
	CHL2MPBotGetProp( void );
	~CHL2MPBotGetProp( void );

	static bool IsPossible( CHL2MPBot* me );	// Return true if this Action has what it needs to perform right now

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot* me, Action< CHL2MPBot >* priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot* me, float interval );
	virtual ActionResult< CHL2MPBot >	OnSuspend( CHL2MPBot* me, Action< CHL2MPBot >* interruptingAction );
	virtual ActionResult< CHL2MPBot >	OnResume( CHL2MPBot* me, Action< CHL2MPBot >* interruptingAction );
	virtual void						OnEnd( CHL2MPBot* me, Action< CHL2MPBot >* nextAction );

	virtual EventDesiredResult< CHL2MPBot > OnContact( CHL2MPBot *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual EventDesiredResult< CHL2MPBot > OnStuck( CHL2MPBot* me );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToSuccess( CHL2MPBot* me, const Path* path );
	virtual EventDesiredResult< CHL2MPBot > OnMoveToFailure( CHL2MPBot* me, const Path* path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot* me ) const;					// are we in a hurry?

	virtual const char* GetName( void ) const { return "GetProp"; };

private:
	PathFollower m_path;
	CHandle< CBaseEntity > m_prop;

	bool m_pushedPhyscannon = false;
};


#endif // HL2MP_BOT_GET_PROP_H
