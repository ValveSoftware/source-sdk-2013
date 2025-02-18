//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_guard_spot.h
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "nav_mesh/tf_path_follower.h"

//---------------------------------------------------------------------------------------------
class CBossAlphaGuardSpot : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual EventDesiredResult< CBossAlpha > OnInjured( CBossAlpha *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "GuardSpot"; }		// return name of this action

private:
	CTFPathFollower m_path;
	CountdownTimer m_lookTimer;
	Vector m_lookAtSpot;
};


#endif // TF_RAID_MODE
