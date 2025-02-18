//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VEHICLE_JEEP_EPISODIC_H
#define VEHICLE_JEEP_EPISODIC_H
#ifdef _WIN32
#pragma once
#endif

#include "vehicle_jeep.h"
#include "ai_basenpc.h"
#include "hl2_vehicle_radar.h"

class CParticleSystem;
class CVehicleCargoTrigger;
class CSprite;

#define NUM_WHEEL_EFFECTS	2
#define NUM_HAZARD_LIGHTS	4

//=============================================================================
// Episodic jeep

class CPropJeepEpisodic : public CPropJeep
{
	DECLARE_CLASS( CPropJeepEpisodic, CPropJeep );
	DECLARE_SERVERCLASS();

public:
					CPropJeepEpisodic( void );

	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual void	Think( void );
	virtual void	UpdateOnRemove( void );

	virtual void	NPC_FinishedEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	virtual void	NPC_FinishedExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	
	virtual bool	NPC_CanEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	virtual bool	NPC_CanExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	Precache( void );
	virtual void	EnterVehicle( CBaseCombatCharacter *pPassenger );
	virtual void	ExitVehicle( int nRole );
	virtual bool	AllowBlockedExit( CBaseCombatCharacter *pPassenger, int nRole );
	
	// Passengers take no damage except what we pass them
	virtual bool	PassengerShouldReceiveDamage( CTakeDamageInfo &info ) 
	{ 
		if ( GetServerVehicle() && GetServerVehicle()->IsPassengerExiting() )
			return false;

		return ( info.GetDamageType() & DMG_VEHICLE ) != 0; 
	}

	virtual int		ObjectCaps( void ) { return (BaseClass::ObjectCaps() | FCAP_NOTIFY_ON_TRANSITION); }

	void	SpawnRadarPanel();
	void	DestroyRadarPanel();
	int		NumRadarContacts() { return m_iNumRadarContacts; }

	void			AddPropToCargoHold( CPhysicsProp *pProp );

	virtual CBaseEntity *OnFailedPhysGunPickup( Vector vPhysgunPos );
	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	virtual int DrawDebugTextOverlays( void );

	DECLARE_DATADESC();

protected:
			void			HazardBlinkThink( void );
			void			CreateHazardLights( void );
			void			DestroyHazardLights( void );

			void			UpdateCargoEntry( void );
			void			ReleasePropFromCargoHold( void );
			void			CreateCargoTrigger( void );
	virtual float			GetUprightTime( void ) { return 1.0f; }
	virtual float			GetUprightStrength( void );
	virtual bool			ShouldPuntUseLaunchForces( PhysGunForce_t reason ) { return ( reason == PHYSGUN_FORCE_PUNTED ); }
	virtual void			HandleWater( void );

	virtual AngularImpulse	PhysGunLaunchAngularImpulse( void );
	virtual Vector			PhysGunLaunchVelocity( const Vector &forward, float flMass );
	bool					PassengerInTransition( void );

	void	SetBusterHopperVisibility(bool visible);

private:
	
	void	UpdateWheelDust( void );
	void	UpdateRadar( bool forceUpdate = false );

	void	InputLockEntrance( inputdata_t &data );
	void	InputUnlockEntrance( inputdata_t &data );
	void	InputLockExit( inputdata_t &data );
	void	InputUnlockExit( inputdata_t &data );
	void	InputEnableRadar( inputdata_t &data );
	void	InputDisableRadar( inputdata_t &data );
	void	InputEnableRadarDetectEnemies( inputdata_t &data );
	void	InputAddBusterToCargo( inputdata_t &data );
	void	InputSetCargoVisibility( inputdata_t &data );
	void	InputOutsideTransition( inputdata_t &data );
	void	InputDisablePhysGun( inputdata_t &data );
	void	InputEnablePhysGun( inputdata_t &data );
	void	InputCreateLinkController( inputdata_t &data );
	void	InputDestroyLinkController( inputdata_t &data );
	void	CreateAvoidanceZone( void );

	bool	m_bEntranceLocked;
	bool	m_bExitLocked;
	bool	m_bAddingCargo;
	bool	m_bBlink;

	float	m_flCargoStartTime;	// Time when the cargo was first added to the vehicle (used for animating into hold)
	float	m_flNextAvoidBroadcastTime; // Next time we'll warn entity to move out of us

	COutputEvent	m_OnCompanionEnteredVehicle;	// Passenger has completed entering the vehicle
	COutputEvent	m_OnCompanionExitedVehicle;		// Passenger has completed exited the vehicle
	COutputEvent	m_OnHostileEnteredVehicle;	// Passenger has completed entering the vehicle
	COutputEvent	m_OnHostileExitedVehicle;		// Passenger has completed exited the vehicle

	CHandle< CParticleSystem >			m_hWheelDust[NUM_WHEEL_EFFECTS];
	CHandle< CParticleSystem >			m_hWheelWater[NUM_WHEEL_EFFECTS];
	CHandle< CVehicleCargoTrigger >		m_hCargoTrigger;
	CHandle< CPhysicsProp >				m_hCargoProp;

	CHandle< CSprite >					m_hHazardLights[NUM_HAZARD_LIGHTS];
	float								m_flNextWaterSound;

	bool	m_bRadarEnabled;
	bool	m_bRadarDetectsEnemies;
	float	m_flNextRadarUpdateTime;
	EHANDLE	m_hRadarScreen;

	EHANDLE	m_hLinkControllerFront;
	EHANDLE m_hLinkControllerRear;

	bool	m_bBusterHopperVisible;	// is the hopper assembly visible on the vehicle? please do not set this directly - use the accessor funct.

	CNetworkVar( int, m_iNumRadarContacts );
	CNetworkArray( Vector, m_vecRadarContactPos, RADAR_MAX_CONTACTS );
	CNetworkArray( int, m_iRadarContactType, RADAR_MAX_CONTACTS );
};

#endif // VEHICLE_JEEP_EPISODIC_H
