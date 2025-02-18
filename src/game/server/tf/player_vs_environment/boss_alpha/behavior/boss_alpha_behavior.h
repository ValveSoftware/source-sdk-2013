//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_behavior.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_BEHAVIOR_H
#define BOSS_ALPHA_BEHAVIOR_H

#ifdef TF_RAID_MODE

//---------------------------------------------------------------------------------------------
class CBossAlphaBehavior : public Action< CBossAlpha >
{
public:
	virtual Action< CBossAlpha > *InitialContainedAction( CBossAlpha *me );

	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );

	virtual EventDesiredResult< CBossAlpha > OnKilled( CBossAlpha *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CBossAlpha > OnContact( CBossAlpha *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action

private:
	CountdownTimer m_vocalTimer;
};


#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_BEHAVIOR_H
