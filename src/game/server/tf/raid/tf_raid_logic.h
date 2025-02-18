//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_raid_logic.h
// Raid game mode singleton manager
// Michael Booth, November 2009

#ifndef TF_RAID_LOGIC_H
#define TF_RAID_LOGIC_H

#ifdef TF_RAID_MODE

#include "tf_gamerules.h"
#include "player_vs_environment/tf_population_manager.h"

class CBaseDoor;


//-----------------------------------------------------------------------
class CRaidLogic : public CPointEntity, public CGameEventListener
{
	DECLARE_CLASS( CRaidLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CRaidLogic();
	virtual ~CRaidLogic();

	virtual void Spawn( void );
	void Reset( void );
	void Update( void );

	virtual void FireGameEvent( IGameEvent *event );

	void DrawDebugDisplay( float deltaT );

	CTeamControlPoint *GetContestedPoint( void ) const;					// return the next control point that can be captured

	bool IsWaitingForRaidersToLeaveSafeRoom( void ) const;				// returns true if all raiders have not yet left the spawn room for the first time
	bool IsMobSpawning( void ) const;									// return true if a mob is trying to spawn right now
	bool IsSentryGunArea( CTFNavArea *area ) const;						// return true if this area should contain an enemy sentry gun

	int GetWandererCount( void ) const;									// how many wanderers exist at the moment?

	CTFPlayer *GetFarthestAlongRaider( void ) const;					// return raider who is farthest through the map

	float GetMaximumRaiderIncursionDistance( void ) const;				// return incursion distance of farthest along raider
	float GetIncursionDistanceAtEnd( void ) const;						// return maximum incursion distance at end of route

	CTFNavArea *GetEscapeRouteStart( void ) const;
	CTFNavArea *GetEscapeRouteEnd( void ) const;

	CTFNavArea *FindSniperSpawn( void );
	CTFNavArea *FindSentryArea( void );

	CTFNavArea *FindSpawnAreaAhead( void );
	CTFNavArea *FindSpawnAreaBehind( void );

	CTFNavArea *SelectRaidSentryArea( void ) const;						// choose unpopulated sentry area nearest the invaders

	CTFPlayer *SelectRaiderToAttack( void );							// pick a member of the raiding (blue) team for a red defender to attack

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CBaseEntity *GetRescueRespawn( void ) const;						// return entity positioned within next valid rescue closet area for to respawn players in

private:
	bool LoadPopulationFromFile( void );

	int m_priorRaiderAliveCount;

	int m_wandererCount;
	int m_engineerCount;
	int m_demomanCount;
	int m_heavyCount;
	int m_soldierCount;
	int m_pyroCount;
	int m_spyCount;
	int m_sniperCount;
	int m_squadCount;

	void OnRoundStart( void );
	bool Unspawn( CTFPlayer *who );
	void CullObsoleteEnemies( float minIncursion, float maxIncursion );

//	void SpawnMobs( CUtlVector< CTFNavArea * > *spawnAreaVector );
	CountdownTimer m_mobSpawnTimer;
	CountdownTimer m_mobLifetimeTimer;
	CTFNavArea *m_mobArea;
	int m_mobCountRemaining;
	int m_mobClass;

//	void SpawnEngineers( void );
	CountdownTimer m_engineerSpawnTimer;

//	void SpawnSpecials( CUtlVector< CTFNavArea * > *spawnAheadVector, CUtlVector< CTFNavArea * > *spawnAnywhereVector );
	CountdownTimer m_specialSpawnTimer;

	CTFNavArea *SelectMobSpawn( CUtlVector< CTFNavArea * > *spawnAreaVector, RelativePositionType where );
//	bool SpawnSquad( CTFNavArea *spawnArea );
	void StartMobTimer( float duration );

	bool m_isWaitingForRaidersToLeaveSpawnRoom;
	bool m_wasCapturingPoint;
	bool m_didFailLastTime;

	CHandle< CTFPlayer > m_farthestAlongRaider;							// raider with highest incursion distance
	float m_incursionDistanceAtEnd;

	void BuildEscapeRoute( void );
	CUtlVector< CTFNavArea * > m_escapeRouteVector;						// a vector of areas in order along the escape route
	CTFNavArea *m_farthestAlongEscapeRouteArea;
	CTFNavArea *FindEarliestVisibleEscapeRouteAreaNearTeam( CTFNavArea *viewArea ) const;	// given a viewing area, return the earliest escape route area near the team that is visible

	CUtlVector< CTFNavArea * > m_sniperSpotVector;						// a vector of good sniping areas
	CUtlVector< CTFNavArea * > m_sentrySpotVector;						// a vector of good sentry areas
	CUtlVector< CTFNavArea * > m_actualSentrySpotVector;				// a vector of areas to actually place sentry guns

	CUtlVector< CTFNavArea * > m_rescueClosetVector;					// a vector of areas to respawn raiders

	CUtlVector< CTFNavArea * > m_miniBossHomeVector;
	int m_miniBossIndex;

	CUtlVector< CBaseDoor * > m_gateVector;								// vector of gates that open when point is captured
};

inline bool CRaidLogic::IsSentryGunArea( CTFNavArea *area ) const
{
	return m_actualSentrySpotVector.HasElement( area );
}

inline int CRaidLogic::GetWandererCount( void ) const
{
	return m_wandererCount;
}

inline CTFNavArea *CRaidLogic::GetEscapeRouteStart( void ) const
{
	return m_escapeRouteVector.Count() ? m_escapeRouteVector[0] : NULL;
}

inline CTFNavArea *CRaidLogic::GetEscapeRouteEnd( void ) const
{
	return m_escapeRouteVector.Count() ? m_escapeRouteVector[ m_escapeRouteVector.Count()-1 ] : NULL;
}

inline CTFPlayer *CRaidLogic::GetFarthestAlongRaider( void ) const
{
	return m_farthestAlongRaider;
}


inline bool CRaidLogic::IsWaitingForRaidersToLeaveSafeRoom( void ) const
{
	return m_isWaitingForRaidersToLeaveSpawnRoom;
}

inline bool CRaidLogic::IsMobSpawning( void ) const
{
	return m_mobLifetimeTimer.HasStarted() && !m_mobLifetimeTimer.IsElapsed();
}

inline float CRaidLogic::GetIncursionDistanceAtEnd( void ) const
{
	return m_incursionDistanceAtEnd;
}

extern CRaidLogic *g_pRaidLogic;

#endif // TF_RAID_MODE


#endif // TF_RAID_LOGIC_H
