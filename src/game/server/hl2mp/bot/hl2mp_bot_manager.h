//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_MANAGER_H
#define HL2MP_BOT_MANAGER_H

#include "NextBotManager.h"
#include "hl2mp_gamerules.h"

class CHL2MPBot;
class CHL2MP_Player;
class CHL2MPBotSquad;
class CStuckBotEvent;



//----------------------------------------------------------------------------------------------
// For parsing and displaying stuck events from server logs.
class CStuckBot
{
public:
	CStuckBot( int id, const char *name )
	{
		m_id = id;
		Q_strncpy( m_name, name, 256 );
	}

	bool IsMatch( int id, const char *name )
	{
		return ( id == m_id && FStrEq( name, m_name ) );
	}

	char m_name[256];
	int m_id;

	CUtlVector< CStuckBotEvent * > m_stuckEventVector;
};



//----------------------------------------------------------------------------------------------
// For parsing and displaying stuck events from server logs.
class CStuckBotEvent
{
public:
	Vector m_stuckSpot;
	float m_stuckDuration;
	Vector m_goalSpot;
	bool m_isGoalValid;

	void Draw( float deltaT = 0.1f )
	{
		NDebugOverlay::Cross3D( m_stuckSpot, 5.0f, 255, 255, 0, true, deltaT );

		if ( m_isGoalValid )
		{
			if ( m_stuckDuration > 6.0f )
			{
				NDebugOverlay::HorzArrow( m_stuckSpot, m_goalSpot, 2.0f, 255, 0, 0, 255, true, deltaT );
			}
			else if ( m_stuckDuration > 3.0f )
			{
				NDebugOverlay::HorzArrow( m_stuckSpot, m_goalSpot, 2.0f, 255, 255, 0, 255, true, deltaT );
			}
			else
			{
				NDebugOverlay::HorzArrow( m_stuckSpot, m_goalSpot, 2.0f, 0, 255, 0, 255, true, deltaT );
			}
		}
	}
};


//----------------------------------------------------------------------------------------------
class CHL2MPBotManager : public NextBotManager
{
public:
	CHL2MPBotManager();
	virtual ~CHL2MPBotManager();

	virtual void Update();
	void LevelShutdown();

	virtual void OnMapLoaded( void );						// when the server has changed maps

	bool IsAllBotTeam( int iTeam );
	bool IsInOfflinePractice() const;
	bool IsMeleeOnly() const;
	bool IsGravGunOnly() const;

	CHL2MPBot* GetAvailableBotFromPool();
	
	void OnForceAddedBots( int iNumAdded );
	void OnForceKickedBots( int iNumKicked );

	void ClearStuckBotData();
	CStuckBot *FindOrCreateStuckBot( int id, const char *playerClass );	// for parsing and debugging stuck bot server logs
	void DrawStuckBotData( float deltaT = 0.1f );

	bool RemoveBotFromTeamAndKick( int nTeam );

protected:
	void MaintainBotQuota();
	void SetIsInOfflinePractice( bool bIsInOfflinePractice );
	void RevertOfflinePracticeConvars();

	float m_flNextPeriodicThink;

	CUtlVector< CStuckBot * > m_stuckBotVector;
	CountdownTimer m_stuckDisplayTimer;
};

// singleton accessor
CHL2MPBotManager &TheHL2MPBots( void );

#endif // HL2MP_BOT_MANAGER_H
