//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_emerge.h
// The Halloween Boss emerging from the ground
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_EMERGE_H
#define EYEBALL_BOSS_EMERGE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossEmerge : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );
	virtual const char *GetName( void ) const	{ return "Emerge"; }		// return name of this action

private:
	CountdownTimer m_riseTimer;
	CountdownTimer m_rumbleTimer;
	CountdownTimer m_killTimer;
	Vector m_emergePos;
	Vector m_groundPos;
	float m_height;
};


#endif // EYEBALL_BOSS_EMERGE_H
