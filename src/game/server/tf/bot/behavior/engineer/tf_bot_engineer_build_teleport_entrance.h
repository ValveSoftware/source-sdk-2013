//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_teleport_entrance.h
// Engineer building a teleport entrance right outside of the spawn room
// Michael Booth, May 2009

#ifndef TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRANCE_H
#define TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRANCE_H

class CTFBotEngineerBuildTeleportEntrance : public Action< CTFBot >
{
public:
	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual EventDesiredResult< CTFBot > OnStuck( CTFBot *me );

	virtual const char *GetName( void ) const	{ return "EngineerBuildTeleportEntrance"; };

private:
	PathFollower m_path;
};

#endif // TF_BOT_ENGINEER_BUILD_TELEPORT_ENTRANCE_H
