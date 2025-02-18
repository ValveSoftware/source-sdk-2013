//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_teleport.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_TELEPORT_H
#define EYEBALL_BOSS_TELEPORT_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossTeleport : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );

	virtual const char *GetName( void ) const	{ return "Teleport"; }		// return name of this action

private:
	enum TeleportState
	{
		TELEPORTING_OUT,
		TELEPORTING_IN,
		DONE
	};
	TeleportState m_state;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CEyeballBossEscape : public Action< CEyeballBoss >
{
public:
	virtual ActionResult< CEyeballBoss >	OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction );
	virtual ActionResult< CEyeballBoss >	Update( CEyeballBoss *me, float interval );

	virtual const char *GetName( void ) const	{ return "Escape"; }		// return name of this action

private:
	CountdownTimer m_timer;
};


#endif // EYEBALL_BOSS_TELEPORT_H
