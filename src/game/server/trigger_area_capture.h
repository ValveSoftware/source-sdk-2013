//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TRIGGER_AREA_CAPTURE_H
#define TRIGGER_AREA_CAPTURE_H
#ifdef _WIN32
#pragma once
#endif

#include "basemultiplayerplayer.h"
#include "triggers.h"
#include "team_control_point.h"

class CTeamTrainWatcher;

#define AREA_ATTEND_TIME 0.7f

#define AREA_THINK_TIME 0.1f

#define CAPTURE_NORMAL					0
#define CAPTURE_CATCHUP_ALIVEPLAYERS	1

#define MAX_CLIENT_AREAS				128
#define MAX_AREA_CAPPERS				9

//-----------------------------------------------------------------------------
// Purpose: An area entity that players must remain in in order to active another entity
//			Triggers are fired on start of capture, on end of capture and on broken capture
//			Can either be capped by both teams at once, or just by one
//			Time to capture and number of people required to capture are both passed by the mapper
//-----------------------------------------------------------------------------
// This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
class CTriggerAreaCaptureShim : public CBaseTrigger
{
	virtual void AreaTouch( CBaseEntity *pOther ) = 0;
public:
	void	Touch( CBaseEntity *pOther ) { return AreaTouch( pOther ) ; }
};

DECLARE_AUTO_LIST( ITriggerAreaCaptureAutoList );

class CTriggerAreaCapture : public CTriggerAreaCaptureShim, public ITriggerAreaCaptureAutoList
{
	DECLARE_CLASS( CTriggerAreaCapture, CTriggerAreaCaptureShim );
public:
	CTriggerAreaCapture();

	// Derived, game-specific area triggers must override these functions
public:
	// Display a hint about capturing zones to the player
	virtual void DisplayCapHintTo( CBaseMultiplayerPlayer *pPlayer ) { return; }

	// A team has finished capturing the zone.
	virtual void OnEndCapture( int iTeam ) { return; }
	virtual void OnStartCapture( int iTeam ) { return; }

public:
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	bool	IsActive( void );
	bool	CheckIfDeathCausesBlock( CBaseMultiplayerPlayer *pVictim, CBaseMultiplayerPlayer *pKiller );

	void	UpdateNumPlayers( bool bBlocked = false );
	void	UpdateOwningTeam( void );
	void	UpdateCappingTeam( int iTeam );
	void	UpdateTeamInZone( void );
	void	UpdateBlocked( void );

	void	ForceOwner( int team ); // by the control_point_round to force an owner of this point (so we can play a specific round)

	bool	TeamCanCap( int iTeam ){ return m_TeamData[iTeam].bCanCap; }
	CHandle<CTeamControlPoint> GetControlPoint( void ){ return m_hPoint; }

	int		GetOwningTeam( void ) { return m_nOwningTeam; }

	bool	IsBlocked( void ) { return m_bBlocked; }

	void	SetTrainWatcher( CTeamTrainWatcher *pTrainWatcher ){ m_hTrainWatcher = pTrainWatcher; } // used for train watchers that control train movement
	CTeamTrainWatcher *GetTrainWatcher( void ) const { return m_hTrainWatcher; }

	virtual void StartTouch(CBaseEntity *pOther) OVERRIDE;
	virtual void EndTouch(CBaseEntity *pOther) OVERRIDE;

	float GetCapTime() const { return m_flCapTime; }

protected:

	virtual bool CaptureModeScalesWithPlayers() const;

private:
	virtual void AreaTouch( CBaseEntity *pOther ) OVERRIDE;
	void	CaptureThink( void );

	void	StartCapture( int team, int capmode );
	void	EndCapture( int team );
	void	BreakCapture( bool bNotEnoughPlayers );
	void	IncrementCapAttemptNumber( void );
	void	SwitchCapture( int team );
	void	SendNumPlayers( void );

	void	SetOwner( int team );	//sets the owner of this point - useful for resetting all to -1
	
	void	InputRoundSpawn( inputdata_t &inputdata );
	void	InputCaptureCurrentCP( inputdata_t &inputdata );
	void	InputSetTeamCanCap( inputdata_t &inputdata );
	void	InputSetControlPoint( inputdata_t &inputdata );
	
	void	SetCapTimeRemaining( float flTime );

	void	HandleRespawnTimeAdjustments( int oldTeam, int newTeam );
	void	GetNumCappingPlayers( int team, int &numcappers, int *cappingplayers );

	void	SetNumCappers( int nNumCappers, bool bBlocked = false );

private:
	int		m_iCapMode;			//which capture mode we're in
	bool	m_bCapturing;
	int		m_nCapturingTeam;	//the team that is capturing this point
	int		m_nOwningTeam;		//the team that has captured this point
	int		m_nTeamInZone;		//if there's one team in the zone, this is it.
	float	m_flCapTime;		//the total time it takes to capture the area, in seconds
	float	m_fTimeRemaining;	//the time left in the capture
	float	m_flLastReductionTime;
	bool	m_bBlocked;

	struct perteamdata_t
	{
		perteamdata_t()
		{
			iNumRequiredToCap = 0;
			iNumTouching = 0;
			iBlockedTouching = 0;
			bCanCap = false;
			iSpawnAdjust = 0;
			iNumRequiredToStartCap = 0;
		}

		int		iNumRequiredToCap;
		int		iNumTouching;
		int		iBlockedTouching;		// Number of capping players on the cap while it's being blocked
		bool	bCanCap;
		int		iSpawnAdjust;
		int		iNumRequiredToStartCap;
	};
	CUtlVector<perteamdata_t>	m_TeamData;

	struct blockers_t
	{
		CHandle<CBaseMultiplayerPlayer>	hPlayer;
		int						iCapAttemptNumber;
		float					flNextBlockTime;
	};
	CUtlVector<blockers_t>	m_Blockers;

	bool	m_bActive;

	COutputEvent m_OnStartTeam1;
	COutputEvent m_OnStartTeam2;
	COutputEvent m_OnBreakTeam1;
	COutputEvent m_OnBreakTeam2;
	COutputEvent m_OnCapTeam1;
	COutputEvent m_OnCapTeam2;

	COutputEvent m_StartOutput;
	COutputEvent m_BreakOutput;
	COutputEvent m_CapOutput;
	
	COutputInt m_OnNumCappersChanged;
	COutputInt m_OnNumCappersChanged2;

	CHandle<CTeamControlPoint>	m_hPoint;	//the capture point that we are linked to!

	bool	m_bRequiresObject;

	string_t m_iszCapPointName;			//name of the cap point that we're linked to

	int	m_iCapAttemptNumber;	// number used to keep track of discrete cap attempts, for block tracking
	bool m_bStartTouch;

	CHandle<CTeamTrainWatcher>	m_hTrainWatcher;	// used for train watchers that control train movement

	DECLARE_DATADESC();
};

#endif // TRIGGER_AREA_CAPTURE_H
