//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_get_off_me.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_GET_OFF_ME_H
#define BOSS_ALPHA_GET_OFF_ME_H

#ifdef TF_RAID_MODE


//----------------------------------------------------------------------------
class CBossAlphaGetOffMe : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void						OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	virtual const char *GetName( void ) const	{ return "GetOffMe"; }		// return name of this action

private:
	CountdownTimer m_timer;
};

#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_GET_OFF_ME_H
