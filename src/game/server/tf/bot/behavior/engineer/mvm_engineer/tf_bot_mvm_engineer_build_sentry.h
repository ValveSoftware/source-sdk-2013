//========= Copyright Valve Corporation, All rights reserved. ============//
// Michael Booth, September 2012

#ifndef TF_BOT_MVM_ENGINEER_BUILD_SENTRYGUN_H
#define TF_BOT_MVM_ENGINEER_BUILD_SENTRYGUN_H

class CTFBotHintSentrygun;

class CTFBotMvMEngineerBuildSentryGun : public Action< CTFBot >
{
public:
	CTFBotMvMEngineerBuildSentryGun( CTFBotHintSentrygun* pSentryHint );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual const char *GetName( void ) const	{ return "MvMEngineerBuildSentryGun"; };

private:
	CHandle< CTFBotHintSentrygun > m_sentryBuildHint;
	CHandle< CObjectSentrygun > m_sentry;

	CountdownTimer m_delayBuildTime;
	CountdownTimer m_repathTimer;
	PathFollower m_path;
};

#endif // TF_BOT_MVM_ENGINEER_BUILD_SENTRYGUN_H
