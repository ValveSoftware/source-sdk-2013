//========= Copyright Valve Corporation, All rights reserved. ============//
////////////////////////////////////////////////////////////////////////////////////////////////////
// tf_bot_training.h
//
// Misc. training actions/behaviors.  To be split up into separate files when we deem them "re-usable"
//
// Tom Bui, April 2010
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TF_BOT_TRAINING_H
#define TF_BOT_TRAINING_H

////////////////////////////////////////////////////////////////////////////////////////////////////
// Attempts to kick/despawn the bot in the Update()

class CTFDespawn : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual const char *GetName( void ) const	{ return "Despawn"; };
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Simple behavior for training where the bot approaches action point and tries to fire at it (and anything there)

class CTFTrainingAttackSentryActionPoint : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual const char *GetName( void ) const	{ return "Despawn"; };

private:
	CountdownTimer m_repathTimer;
	PathFollower m_path;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Tells a bot to go an Action Point and run any command it has
class CTFGotoActionPoint : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual const char *GetName( void ) const	{ return "GotoActionPoint"; };

private:
	CountdownTimer m_stayTimer;
	CountdownTimer m_repathTimer;
	PathFollower m_path;
	bool m_wasTeleported;
};

#endif // TF_BOT_TRAINING_H
