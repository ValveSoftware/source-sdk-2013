//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_lost_victim.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_LOST_VICTIM_H
#define BOSS_ALPHA_LOST_VICTIM_H

#ifdef TF_RAID_MODE

//---------------------------------------------------------------------------------------------
class CBossAlphaLostVictim : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	virtual const char *GetName( void ) const	{ return "LostVictim"; }		// return name of this action

private:
	CountdownTimer m_timer;
	float m_headTurn;
	int m_headYawPoseParameter;
};

#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_LOST_VICTIM_H
