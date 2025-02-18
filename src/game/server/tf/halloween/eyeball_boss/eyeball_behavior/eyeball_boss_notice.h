//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_notice.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_NOTICE_H
#define EYEBALL_BOSS_NOTICE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossNotice : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss > OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss > Update( CEyeballBoss *me, float interval );

	virtual const char *GetName( void ) const	{ return "Notice"; }		// return name of this action

private:
	CountdownTimer m_timer;
};

#endif // EYEBALL_BOSS_NOTICE_H
