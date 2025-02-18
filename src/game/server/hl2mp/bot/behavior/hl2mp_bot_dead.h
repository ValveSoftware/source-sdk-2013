//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_DEAD_H
#define HL2MP_BOT_DEAD_H

#include "Path/NextBotChasePath.h"

class CHL2MPBotDead : public Action< CHL2MPBot >
{
public:
	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "Dead"; };

private:
	IntervalTimer m_deadTimer;
};

#endif // HL2MP_BOT_DEAD_H
