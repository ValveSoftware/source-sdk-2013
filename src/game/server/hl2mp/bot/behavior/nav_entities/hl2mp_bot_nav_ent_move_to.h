//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_NAV_ENT_MOVE_TO_H
#define HL2MP_BOT_NAV_ENT_MOVE_TO_H

#include "Path/NextBotPathFollow.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"


class CHL2MPBotNavEntMoveTo : public Action< CHL2MPBot >
{
public:
	CHL2MPBotNavEntMoveTo( const CFuncNavPrerequisite *prereq );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "NavEntMoveTo"; };

private:
	CHandle< CFuncNavPrerequisite > m_prereq;
	Vector m_goalPosition;				// specific position within entity to move to
	CNavArea* m_pGoalArea;

	CountdownTimer m_waitTimer;

	PathFollower m_path;				// how we get to the loot
	CountdownTimer m_repathTimer;
};


#endif // HL2MP_BOT_NAV_ENT_MOVE_TO_H
