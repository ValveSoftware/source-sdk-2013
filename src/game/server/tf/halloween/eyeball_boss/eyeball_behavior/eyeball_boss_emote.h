//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_emote.h
// The 2011 Halloween Boss - play an animation
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_EMOTE_H
#define EYEBALL_BOSS_EMOTE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossEmote : public Action< CEyeballBoss >
{
public:
	CEyeballBossEmote( int animationSequence, const char *soundName, Action< CEyeballBoss > *nextAction = NULL );
	virtual ~CEyeballBossEmote() { }

	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );

	virtual const char *GetName( void ) const	{ return "Emote"; }		// return name of this action

private:
	int m_animationSequence;
	const char *m_soundName;
	Action< CEyeballBoss > *m_nextAction;
};

#endif // EYEBALL_BOSS_EMOTE_H
