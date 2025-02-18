//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_NAV_ENT_DESTROY_ENTITY_H
#define HL2MP_BOT_NAV_ENT_DESTROY_ENTITY_H

#include "Path/NextBotPathFollow.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"
#include "hl2mp/weapon_slam.h"

class CHL2MPBotNavEntDestroyEntity : public Action< CHL2MPBot >
{
public:
	CHL2MPBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq );

	virtual ActionResult< CHL2MPBot >	OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction );
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot *me, float interval );
	virtual void					OnEnd( CHL2MPBot *me, Action< CHL2MPBot > *nextAction );

	virtual const char *GetName( void ) const	{ return "NavEntDestroyEntity"; };

private:
	CHandle< CFuncNavPrerequisite > m_prereq;
	PathFollower m_path;				// how we get to the target
	CountdownTimer m_repathTimer;
	bool m_wasIgnoringEnemies;

	void DetonateStickiesWhenSet( CHL2MPBot *me, CWeapon_SLAM *stickyLauncher ) const;
	bool m_isReadyToLaunchSticky;
};


#endif // HL2MP_BOT_NAV_ENT_DESTROY_ENTITY_H
