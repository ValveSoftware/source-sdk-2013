//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_tactical_mission.h
// Team Fortress specific missions
// Michael Booth, July 2009

#ifndef TF_TACTICAL_MISSION_H
#define TF_TACTICAL_MISSION_H

#include "tactical_mission.h"
#include "team_control_point_master.h"
#include "fmtstr.h"

class CTFDefendSetupGatesDeployZone : public CTacticalMissionZone
{
public:

};

class CTFDefendSetupGatesMission : public CTacticalMission
{
public:
	CTFDefendSetupGatesMission( void );

	virtual const CTacticalMissionZone *GetDeployZone( CBasePlayer *who ) const;	// where give player should be during this mission
	virtual const CTacticalMissionZone *GetObjectiveZone( void ) const;				// control points, setup gates, sections of cart path, etc.
	virtual const CTacticalMissionZone *GetEnemyZone( void ) const;					// where we expect enemies to be during this mission

	virtual const char *GetName( void ) const	{ return "DefendSetupGates"; }		// return name of this mission
};


//---------------------------------------------------------------------------------------------
class CTFDefendPointZone : public CTacticalMissionZone
{
public:
	CTFDefendPointZone( CTeamControlPoint *point );
};


class CTFDefendPointSniperZone : public CTacticalMissionZone
{
public:
	CTFDefendPointSniperZone( CTeamControlPoint *point );
	virtual ~CTFDefendPointSniperZone( void ){}
};


class CTFDefendPointMission : public CTacticalMission
{
public:
	CTFDefendPointMission( CTeamControlPoint *point );
	~CTFDefendPointMission( void );

	virtual const CTacticalMissionZone *GetDeployZone( CBasePlayer *who ) const;	// where give player should be during this mission
	virtual const CTacticalMissionZone *GetObjectiveZone( void ) const;				// control points, setup gates, sections of cart path, etc.
	virtual const CTacticalMissionZone *GetEnemyZone( void ) const;					// where we expect enemies to be during this mission

	virtual const char *GetName( void ) const	{ return m_name; }				// return name of this mission

private:
	CTeamControlPoint *m_point;

	CTFDefendPointZone *m_defenseZone;
	CTFDefendPointSniperZone *m_sniperZone;

	CFmtStrN< 32 > m_name;
};


//---------------------------------------------------------------------------------------------
/**
 * The mission manager provides access to all available missions
 */
class CTFTacticalMissionManager : public CTacticalMissionManager
{
public:
	virtual void OnServerActivate( void );								// Invoked when server loads a new map, after everything has been created/spawned
	virtual void OnRoundRestart( void ); 								// invoked when a game round restarts
};

#endif // TF_TACTICAL_MISSION_H
