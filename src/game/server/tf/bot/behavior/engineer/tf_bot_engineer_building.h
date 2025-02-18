//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_building.h
// At building location, constructing buildings
// Michael Booth, May 2010

#ifndef TF_BOT_ENGINEER_BUILDING_H
#define TF_BOT_ENGINEER_BUILDING_H

class CTFBotHintSentrygun;


class CTFBotEngineerBuilding : public Action< CTFBot >
{
public:
	CTFBotEngineerBuilding( void );
	CTFBotEngineerBuilding( CTFBotHintSentrygun *sentryBuildHint );

	virtual ActionResult< CTFBot >	OnStart( CTFBot *me, Action< CTFBot > *priorAction );
	virtual ActionResult< CTFBot >	Update( CTFBot *me, float interval );
	virtual void					OnEnd( CTFBot *me, Action< CTFBot > *nextAction );

	virtual ActionResult< CTFBot >	OnResume( CTFBot *me, Action< CTFBot > *interruptingAction );

	virtual EventDesiredResult< CTFBot > OnTerritoryLost( CTFBot *me, int territoryID );
	virtual EventDesiredResult< CTFBot > OnTerritoryCaptured( CTFBot *me, int territoryID );

	virtual const char *GetName( void ) const	{ return "EngineerBuilding"; };

private:
	CountdownTimer m_searchTimer;
	CountdownTimer m_getAmmoTimer;
	CountdownTimer m_repathTimer;
	CountdownTimer m_buildTeleporterExitTimer;

	int m_sentryTriesLeft;

	CountdownTimer m_dispenserRetryTimer;
	CountdownTimer m_teleportExitRetryTimer;

	PathFollower m_path;

	CHandle< CTFBotHintSentrygun > m_sentryBuildHint;

	bool m_hasBuiltSentry;

	enum NearbyMetalType
	{
		NEARBY_METAL_UNKNOWN,
		NEARBY_METAL_NONE,
		NEARBY_METAL_EXISTS
	};

	NearbyMetalType m_nearbyMetalStatus;

	CountdownTimer m_territoryRangeTimer;
	bool m_isSentryOutOfPosition;
	bool CheckIfSentryIsOutOfPosition( CTFBot *me ) const;

	void UpgradeAndMaintainBuildings( CTFBot *me );
	bool IsMetalSourceNearby( CTFBot *me ) const;
};


#endif // TF_BOT_ENGINEER_BUILDING_H
