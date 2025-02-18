//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_nuke_attack.h
// Michael Booth, November 2010

#ifndef BOSS_ALPHA_NUKE_ATTACK_H
#define BOSS_ALPHA_NUKE_ATTACK_H

#ifdef TF_RAID_MODE

class CBossAlphaNukeAttack : public Action< CBossAlpha >
{
public:
	virtual ActionResult< CBossAlpha >	OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction );
	virtual ActionResult< CBossAlpha >	Update( CBossAlpha *me, float interval );
	virtual void					OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBossAlpha >	OnSuspend( CBossAlpha *me, Action< CBossAlpha > *interruptingAction )	{ return Done(); }

	virtual EventDesiredResult< CBossAlpha > OnInjured( CBossAlpha *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "NukeAttack"; }		// return name of this action

private:
	CountdownTimer m_shakeTimer;
	CountdownTimer m_chargeUpTimer;
};

#endif // TF_RAID_MODE

#endif // BOSS_ALPHA_NUKE_ATTACK_H
