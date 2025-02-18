//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_NAV_ENT_WAIT_H
#define HL2MP_BOT_NAV_ENT_WAIT_H

#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"


class CHL2MPBotNavEntWait : public Action< CHL2MPBot >
{
public:
	CHL2MPBotNavEntWait( const CFuncNavPrerequisite *prereq );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "NavEntWait"; };

private:
	CHandle< CFuncNavPrerequisite > m_prereq;
	CountdownTimer m_timer;
};


#endif // HL2MP_BOT_NAV_ENT_WAIT_H
