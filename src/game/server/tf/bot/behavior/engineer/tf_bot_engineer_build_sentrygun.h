//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_sentrygun.h
// Engineer building his Sentry gun
// Michael Booth, May 2010

#ifndef TF_BOT_ENGINEER_BUILD_SENTRYGUN_H
#define TF_BOT_ENGINEER_BUILD_SENTRYGUN_H

class CTFBotHintSentrygun;


class CTFBotEngineerBuildSentryGun : public Action< CTFBot >
{
public:
	CTFBotEngineerBuildSentryGun( void );
	CTFBotEngineerBuildSentryGun( CTFBotHintSentrygun *sentryBuildHint );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual const char *GetName( void ) const	{ return "EngineerBuildSentryGun"; };

private:
	CountdownTimer m_searchTimer;
	CountdownTimer m_giveUpTimer;
	CountdownTimer m_getAmmoTimer;
	CountdownTimer m_repathTimer;
	CountdownTimer m_buildTeleporterExitTimer;

	int m_sentryTriesLeft;
	PathFollower m_path;

	CTFBotHintSentrygun *m_sentryBuildHint;
	Vector m_sentryBuildLocation;

	int m_wanderWay;
	bool m_needToAimSentry;
	Vector m_sentryBuildAimTarget;
};


#endif // TF_BOT_ENGINEER_BUILD_SENTRYGUN_H
