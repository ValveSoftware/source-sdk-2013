//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_companion.h
// Teammate bots for Raid mode
// Michael Booth, October 2009

#ifndef TF_BOT_COMPANION_H
#define TF_BOT_COMPANION_H

#ifdef TF_RAID_MODE

#include "Path/NextBotPathFollow.h"
#include "Path/NextBotChasePath.h"

//
// Friendly teammate bots
//
class CTFBotCompanion : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual const char *GetName( void ) const	{ return "Companion"; };

private:
	ChasePath m_path;
	CTFPlayer *GetLeader( void );
};


//
// Friendly defenders of the base
//
class CTFBotGuardian : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );
	virtual EventDesiredResult< CTFBot > OnMoveToSuccess( CTFBot *me, const Path *path );
	virtual EventDesiredResult< CTFBot > OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "Guardian"; };

private:
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

#endif // TF_RAID_MODE

#endif // TF_BOT_COMPANION_H
