//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_teleport_exit.h
// Engineer building a teleport exit
// Michael Booth, May 2010

#ifndef TF_BOT_ENGINEER_BUILD_TELEPORT_EXIT_H
#define TF_BOT_ENGINEER_BUILD_TELEPORT_EXIT_H

class CTFBotEngineerBuildTeleportExit : public Action< CTFBot >
{
public:
	CTFBotEngineerBuildTeleportExit( void );
	CTFBotEngineerBuildTeleportExit( const Vector &buildLocation, float buildAngle );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );

	virtual const char *GetName( void ) const	{ return "EngineerBuildTeleportExit"; };

private:
	PathFollower m_path;

	bool m_hasPreciseBuildLocation;
	Vector m_buildLocation;
	float m_buildAngle;

	CountdownTimer m_giveUpTimer;
	CountdownTimer m_repathTimer;
	CountdownTimer m_getAmmoTimer;
	CountdownTimer m_searchTimer;
};

#endif // TF_BOT_ENGINEER_BUILD_TELEPORT_EXIT_H
