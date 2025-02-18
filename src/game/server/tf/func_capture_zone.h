//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag Capture Zone.
//
//=============================================================================//
#ifndef FUNC_CAPTURE_ZONE_H
#define FUNC_CAPTURE_ZONE_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

// This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
class CCaptureZoneShim : public CBaseTrigger
{
	virtual void ShimTouch( CBaseEntity *pOther ) = 0;
public:
	void	Touch( CBaseEntity *pOther ) { return ShimTouch( pOther ) ; }
};

//=============================================================================
//
// CTF Flag Capture Zone class.
//
DECLARE_AUTO_LIST( ICaptureZoneAutoList );
class CCaptureZone : public CCaptureZoneShim, public ICaptureZoneAutoList
{
	DECLARE_CLASS( CCaptureZone, CCaptureZoneShim );

public:
	DECLARE_SERVERCLASS();

	CCaptureZone();

	void	Spawn();
	virtual void	Activate();
	virtual void	ShimTouch( CBaseEntity *pOther ) OVERRIDE;

	void	Capture( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	int		UpdateTransmitState( void );

	void	PlayerDestructionThink( void );

private:

	CNetworkVar( bool, m_bDisabled );		// Enabled/Disabled?
	
	int				m_nCapturePoint;	// Used in non-CTF maps to identify this capture point

	COutputEvent	m_outputOnCapture;	// Fired a flag is captured on this point.
	COutputEvent	m_OnCapTeam1;
	COutputEvent	m_OnCapTeam2;
	COutputEvent	m_OnCapTeam1_PD;
	COutputEvent	m_OnCapTeam2_PD;

	DECLARE_DATADESC();

	float			m_flNextTouchingEnemyZoneWarning;	// don't spew warnings to the player who is touching the wrong cap
	bool			m_bShouldBlock;
	float			m_flCaptureDelay;
	float			m_flCaptureDelayOffset;
};


//=============================================================================
//
// CTF Flag Detection Zone class.
//
DECLARE_AUTO_LIST( IFlagDetectionZoneAutoList );
class CFlagDetectionZone : public CBaseTrigger, public IFlagDetectionZoneAutoList
{
	DECLARE_CLASS( CFlagDetectionZone, CBaseTrigger );

public:

	CFlagDetectionZone();

	void	Spawn();
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );

	bool	IsDisabled( void ) { return m_bDisabled; }
	void	SetDisabled( bool bDisabled );
	bool	IsAlarmZone( void ){ return m_bShouldAlarm; }

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputTest( inputdata_t &inputdata );

	void	FlagDropped( CBasePlayer *pPlayer );
	void	FlagPickedUp( CBasePlayer *pPlayer );
	void	FlagCaptured( CBasePlayer *pPlayer );

private:

	bool	EntityIsFlagCarrier( CBaseEntity *pEntity );

	bool	m_bDisabled;		// Enabled/Disabled?
	bool	m_bShouldAlarm;

	COutputEvent	m_outputOnStartTouchFlag;	// Fired when a flag or player holding the flag touches
	COutputEvent	m_outputOnEndTouchFlag;
	COutputEvent	m_outputOnDroppedFlag;
	COutputEvent	m_outputOnPickedUpFlag;

	// Entities currently being touched by this trigger
	CUtlVector< EHANDLE >	m_hTouchingPlayers;

	DECLARE_DATADESC();
};

// Fire output for detection zones if entity is in a the zone
void HandleFlagDroppedInDetectionZone( CBasePlayer *pPlayer );
void HandleFlagPickedUpInDetectionZone( CBasePlayer *pPlayer );
void HandleFlagCapturedInDetectionZone( CBasePlayer *pPlayer );


#endif // FUNC_CAPTURE_ZONE_H
