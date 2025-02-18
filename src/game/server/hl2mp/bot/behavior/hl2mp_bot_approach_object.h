//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_APPROACH_OBJECT_H
#define HL2MP_BOT_APPROACH_OBJECT_H

#include "Path/NextBotPathFollow.h"

class CHL2MPBotApproachObject : public Action< CHL2MPBot >
{
public:
	CHL2MPBotApproachObject( CBaseEntity *loot, float range = 10.0f );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "ApproachObject"; };

private:
	CHandle< CBaseEntity > m_loot;		// what we are collecting
	float m_range;						// how close should we get
	PathFollower m_path;				// how we get to the loot
	CountdownTimer m_repathTimer;
};


#endif // HL2MP_BOT_APPROACH_OBJECT_H
