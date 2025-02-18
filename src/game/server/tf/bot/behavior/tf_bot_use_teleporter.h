//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_use_teleporter.h
// Ride a friendly teleporter
// Michael Booth, May 2010

#ifndef TF_BOT_USE_TELEPORTER_H
#define TF_BOT_USE_TELEPORTER_H

#include "tf_obj_teleporter.h"
#include "Path/NextBotPathFollow.h"

class CTFBotUseTeleporter : public Action< CTFBot >
{
public:
	enum UseHowType
	{
		USE_IF_READY,
		ALWAYS_USE
	};
	CTFBotUseTeleporter( CObjectTeleporter *teleporter, UseHowType how = USE_IF_READY );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "UseTeleporter"; };

private:
	CHandle< CObjectTeleporter > m_teleporter;		// the teleporter we're trying to use
	UseHowType m_how;

	PathFollower m_path;
	CountdownTimer m_repathTimer;

	bool m_isInTransit;

	bool IsTeleporterAvailable( void ) const;
};


#endif // TF_BOT_USE_TELEPORTER_H
