//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================

#ifndef TF_BOT_MVM_ENGINEER_TELEPORT_SPAWN_H
#define TF_BOT_MVM_ENGINEER_TELEPORT_SPAWN_H

class CBaseTFBotHintEntity;

class CTFBotMvMEngineerTeleportSpawn : public Action< CTFBot >
{
public:
	CTFBotMvMEngineerTeleportSpawn( CBaseTFBotHintEntity* pHint, bool bFirstTeleportSpawn );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "MvMEngineerTeleportSpawn"; };

private:
	CountdownTimer m_teleportDelay;
	CHandle< CBaseTFBotHintEntity > m_hintEntity;
	bool m_bFirstTeleportSpawn;
};

#endif // TF_BOT_MVM_ENGINEER_TELEPORT_SPAWN_H
