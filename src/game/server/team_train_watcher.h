//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TEAM_TRAIN_WATCHER_H
#define TEAM_TRAIN_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "trigger_area_capture.h"
#include "shareddefs.h"
#include "envspark.h"
#include "GameEventListener.h"

class CFuncTrackTrain;
class CPathTrack;
class CTeamControlPoint;

#define TEAM_TRAIN_ALERT_DISTANCE	750   // alert is the VO warning
#define TEAM_TRAIN_ALARM_DISTANCE	200   // alarm is the looping sound played at the control point

#define TEAM_TRAIN_ALERT			"Announcer.Cart.Warning"
#define TEAM_TRAIN_FINAL_ALERT		"Announcer.Cart.FinalWarning"
#define TEAM_TRAIN_ALARM			"Cart.Warning"
#define TEAM_TRAIN_ALARM_SINGLE		"Cart.WarningSingle"

#define TW_THINK		"CTeamTrainWatcherThink"
#define TW_ALARM_THINK	"CTeamTrainWatcherAlarmThink"
#define TW_ALARM_THINK_INTERVAL	8.0

// #define TWMASTER_THINK	"CTeamTrainWatcherMasterThink"

DECLARE_AUTO_LIST( ITFTeamTrainWatcher );

class CTeamTrainWatcher : public CBaseEntity, public CGameEventListener, public ITFTeamTrainWatcher
{
	DECLARE_CLASS( CTeamTrainWatcher, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTeamTrainWatcher();
	~CTeamTrainWatcher();

	virtual void UpdateOnRemove( void );
	virtual int UpdateTransmitState();

	void InputRoundActivate( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void InputSetNumTrainCappers( inputdata_t &inputdata );
	void InputOnStartOvertime( inputdata_t &inputdata );
	void InputSetSpeedForwardModifier( inputdata_t &inputdata );
	void InputSetTrainRecedeTime( inputdata_t &inputdata );
	void InputSetTrainCanRecede( inputdata_t &inputdata );
	void InputSetTrainRecedeTimeAndUpdate( inputdata_t &inputdata );

	// ==========================================================
	// given a start node and a list of goal nodes
	// calculate the distance between each
	// ==========================================================
	void WatcherActivate( void );

	void WatcherThink( void );
	void WatcherAlarmThink( void );

	CBaseEntity *GetTrainEntity( void );
	bool IsDisabled( void ) { return m_bDisabled; }

	bool TimerMayExpire( void );

	void StopCaptureAlarm( void );

	void SetNumTrainCappers( int iNumCappers, CBaseEntity *pTrigger );  // only used for train watchers that control the train movement

	virtual void FireGameEvent( IGameEvent * event );

	int GetCapturerCount( void ) const;			// return the number of players who are "capturing" the payload, or -1 if the payload is blocked

	void ProjectPointOntoPath( const Vector &pos, Vector *posOnPath, float *distanceAlongPath ) const;	// project the given position onto the track and return the point and how far along that projected position is
	bool IsAheadOfTrain( const Vector &pos ) const;	// return true if the given position is farther down the track than the train is

	bool IsTrainAtStart( void ) const;				// return true if the train hasn't left its starting position yet
	bool IsTrainNearCheckpoint( void ) const;		// return true if the train is almost at the next checkpoint

	float GetTrainDistanceAlongTrack( void ) const;
	Vector GetNextCheckpointPosition( void ) const;	// return world space location of next checkpoint along the path


	float GetTrainProgress() { return m_flTotalProgress; }

private:

	void StartCaptureAlarm( CTeamControlPoint *pPoint );
	void PlayCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap );
	void InternalSetNumTrainCappers( int iNumCappers, CBaseEntity *pTrigger );
	void InternalSetSpeedForwardModifier( float flModifier );
#ifdef GLOWS_ENABLE
	void FindGlowEntity( void );
#endif // GLOWS_ENABLE
	void HandleTrainMovement( bool bStartReceding = false );
	void HandleSparks( bool bSparks );

private:

	bool m_bDisabled;
	bool m_bTrainCanRecede;
	// === Data ===

	// pointer to the train that we're checking
	CHandle<CFuncTrackTrain> m_hTrain;

	// start node
	CHandle<CPathTrack>	m_hStartNode;

	// goal node
	CHandle<CPathTrack>	m_hGoalNode;

	string_t m_iszTrain;
	string_t m_iszStartNode;
	string_t m_iszGoalNode;

	// list of node associations with control points
	typedef struct 
	{
		CHandle<CPathTrack>	hPathTrack;
		CHandle<CTeamControlPoint> hCP;
		float flDistanceFromStart;
		bool bAlertPlayed;
	} node_cp_pair_t;

	node_cp_pair_t m_CPLinks[MAX_CONTROL_POINTS];
	int m_iNumCPLinks;

	string_t m_iszLinkedPathTracks[MAX_CONTROL_POINTS];
	string_t m_iszLinkedCPs[MAX_CONTROL_POINTS];

	float m_flTotalPathDistance;	// calculated only at round start, node graph
	// may get chopped as the round progresses

	float m_flTrainDistanceAccumulator;
	float m_flTrainDistanceFromStart;	// actual distance along path of train, for comparing against m_CPLinks[].flDistanceFromStart

	float m_flSpeedLevels[3];

	// === Networked Data ===

	// current total progress, percentage
	CNetworkVar( float, m_flTotalProgress );

	CNetworkVar( int, m_iTrainSpeedLevel );

	CNetworkVar( int, m_nNumCappers );

	bool m_bWaitingToRecede;
	CNetworkVar( float, m_flRecedeTime );
	float m_flRecedeTotalTime;
	float m_flRecedeStartTime;
	COutputEvent m_OnTrainStartRecede;

	bool m_bCapBlocked;

	float m_flNextSpeakForwardConceptTime; // used to have players speak the forward concept every X seconds
	CHandle<CTriggerAreaCapture> m_hAreaCap;

	CSoundPatch *m_pAlarm;
	float m_flAlarmEndTime;
	bool m_bAlarmPlayed;

	// added for new mode where the train_watcher handles the train movement
	bool m_bHandleTrainMovement;
	string_t m_iszSparkName;
	CUtlVector< CHandle<CEnvSpark> > m_Sparks;
	float m_flSpeedForwardModifier;
	int m_iCurrentHillType;
	float m_flCurrentSpeed;
	bool m_bReceding;

	int m_nTrainRecedeTime;

#ifdef GLOWS_ENABLE
	CNetworkVar( EHANDLE, m_hGlowEnt );
#endif // GLOWS_ENABLE
};


inline float CTeamTrainWatcher::GetTrainDistanceAlongTrack( void ) const
{
	return m_flTrainDistanceFromStart;
}

inline int CTeamTrainWatcher::GetCapturerCount( void ) const
{
	return m_nNumCappers;
}


/*
class CTeamTrainWatcherMaster : public CBaseEntity, public CGameEventListener
{
	DECLARE_CLASS( CTeamTrainWatcherMaster, CBaseEntity );

public:
	CTeamTrainWatcherMaster();
	~CTeamTrainWatcherMaster();

	void Precache( void );

private:
	void TWMThink( void );
	void FireGameEvent( IGameEvent *event );

	bool FindTrainWatchers( void );

private:
	CTeamTrainWatcher *m_pBlueWatcher;
	CTeamTrainWatcher *m_pRedWatcher;

	float m_flBlueProgress;
	float m_flRedProgress;
};

extern EHANDLE g_hTeamTrainWatcherMaster;
*/

#endif //TEAM_TRAIN_WATCHER_H
