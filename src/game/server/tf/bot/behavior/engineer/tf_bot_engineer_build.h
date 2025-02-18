//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build.h
// Engineer building his buildings
// Michael Booth, February 2009

#ifndef TF_BOT_ENGINEER_BUILD_H
#define TF_BOT_ENGINEER_BUILD_H

class CTFBotHintTeleporterExit;


class CTFBotEngineerBuild : public Action< CTFBot >
{
public:
	virtual Action< CTFBot > *InitialContainedAction( CTFBot *me );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnTerritoryLost( CTFBot *me, int territoryID );

	virtual QueryResultType	ShouldHurry( const INextBot *me ) const;							// are we in a hurry?
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?

	virtual const char *GetName( void ) const	{ return "EngineerBuild"; };
};


#endif // TF_BOT_ENGINEER_BUILD_H
