//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_nav_ent_destroy_entity.h
// Destroy the given entity, under nav entity control
// Michael Booth, September 2009

#ifndef TF_BOT_NAV_ENT_DESTROY_ENTITY_H
#define TF_BOT_NAV_ENT_DESTROY_ENTITY_H

#include "Path/NextBotPathFollow.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"
#include "tf_weapon_pipebomblauncher.h"

class CTFBotNavEntDestroyEntity : public Action< CTFBot >
{
public:
	CTFBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual const char *GetName( void ) const	{ return "NavEntDestroyEntity"; };

private:
	CHandle< CFuncNavPrerequisite > m_prereq;
	PathFollower m_path;				// how we get to the target
	CountdownTimer m_repathTimer;
	bool m_wasIgnoringEnemies;

	void DetonateStickiesWhenSet( CTFBot *me, CTFPipebombLauncher *stickyLauncher ) const;
	bool m_isReadyToLaunchSticky;
};


#endif // TF_BOT_NAV_ENT_DESTROY_ENTITY_H
