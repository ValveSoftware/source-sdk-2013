//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_stunned.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_STUNNED_H
#define BOSS_ALPHA_STUNNED_H

#ifdef TF_RAID_MODE

class CBossAlphaStunned : public Action< CBossAlpha >
{
public:
	CBossAlphaStunned( float duration, Action< CBossAlpha > *nextAction = NULL );

	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	virtual EventDesiredResult< CBossAlpha > OnInjured( CBossAlpha *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Stunned"; }		// return name of this action

private:
	CountdownTimer m_timer;
	enum StunStateType
	{
		BECOMING_STUNNED,
		STUNNED,
		RECOVERING
	}
	m_state;
	int m_layerUsed;

	Action< CBossAlpha > *m_nextAction;
};

#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_STUNNED_H
