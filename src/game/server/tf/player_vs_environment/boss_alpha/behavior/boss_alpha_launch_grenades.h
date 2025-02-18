//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_launch_grenades.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_LAUNCH_GRENADES_H
#define BOSS_ALPHA_LAUNCH_GRENADES_H

#ifdef TF_RAID_MODE

#include "tf_weapon_grenade_pipebomb.h"

class CBossAlphaLaunchGrenades : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBossAlpha >	OnSuspend( CBossAlpha *me, Action< CBossAlpha > *interruptingAction )	{ return Done(); }

	virtual const char *GetName( void ) const	{ return "LaunchGrenades"; }		// return name of this action

private:
	CountdownTimer m_timer;
	CountdownTimer m_detonateTimer;
	CUtlVector< CHandle< CTFGrenadePipebombProjectile > > m_grenadeVector;
	void LaunchGrenade( CBossAlpha *me, const Vector &launchVel, CTFWeaponInfo *weaponInfo );
	void LaunchGrenadeRings( CBossAlpha *me );
	void LaunchGrenadeSpokes( CBossAlpha *me );
	int m_animLayer;
};

#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_LAUNCH_GRENADES_H
