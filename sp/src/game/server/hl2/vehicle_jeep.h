//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VEHICLE_JEEP_H
#define VEHICLE_JEEP_H
#ifdef _WIN32
#pragma once
#endif

#include "vehicle_base.h"

#define JEEP_WHEEL_COUNT	4

struct JeepWaterData_t
{
	bool		m_bWheelInWater[JEEP_WHEEL_COUNT];
	bool		m_bWheelWasInWater[JEEP_WHEEL_COUNT];
	Vector		m_vecWheelContactPoints[JEEP_WHEEL_COUNT];
	float		m_flNextRippleTime[JEEP_WHEEL_COUNT];
	bool		m_bBodyInWater;
	bool		m_bBodyWasInWater;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPropJeep : public CPropVehicleDriveable
{
public:
	DECLARE_CLASS( CPropJeep, CPropVehicleDriveable );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPropJeep( void );

	// CPropVehicle
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	virtual bool	AllowBlockedExit( CBasePlayer *pPlayer, int nRole ) { return false; }
	virtual bool	CanExitVehicle( CBaseEntity *pEntity );
	virtual bool	IsVehicleBodyInWater() { return m_WaterData.m_bBodyInWater; }
	
	// Passengers do not directly receive damage from blasts or radiation damage
	virtual bool PassengerShouldReceiveDamage( CTakeDamageInfo &info ) 
	{ 
		if ( GetServerVehicle() && GetServerVehicle()->IsPassengerExiting() )
			return false;

		if ( info.GetDamageType() & DMG_VEHICLE )
			return true;

		return (info.GetDamageType() & (DMG_RADIATION|DMG_BLAST) ) == 0; 
	}

	// CBaseEntity
	void			Think(void);
	void			Precache( void );
	void			Spawn( void ); 
	void			Activate( void );

	virtual void	CreateServerVehicle( void );
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual float	PassengerDamageModifier( const CTakeDamageInfo &info );

	virtual void	EnterVehicle( CBaseCombatCharacter *pPassenger );
	virtual void	ExitVehicle( int nRole );

	void			AimGunAt( Vector *endPos, float flInterval );
	bool			TauCannonHasBeenCutOff( void ) { return m_bGunHasBeenCutOff; }

	// NPC Driving
	bool			NPC_HasPrimaryWeapon( void ) { return true; }
	void			NPC_AimPrimaryWeapon( Vector vecTarget );

	const char		*GetTracerType( void ) { return "AR2Tracer"; }
	void			DoImpactEffect( trace_t &tr, int nDamageType );

	bool HeadlightIsOn( void ) { return m_bHeadlightIsOn; }
	void HeadlightTurnOn( void ) { m_bHeadlightIsOn = true; }
	void HeadlightTurnOff( void ) { m_bHeadlightIsOn = false; }

private:

	void		FireCannon( void );
	void		ChargeCannon( void );
	void		FireChargedCannon( void );

	void		DrawBeam( const Vector &startPos, const Vector &endPos, float width );
	void		StopChargeSound( void );
	void		GetCannonAim( Vector *resultDir );

	void		InitWaterData( void );
	void		CheckWaterLevel( void );
	void		CreateSplash( const Vector &vecPosition );
	void		CreateRipple( const Vector &vecPosition );

	void		CreateDangerSounds( void );

	void		ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );
	void		DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void		DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );

	void		JeepSeagullThink( void );
	void		SpawnPerchedSeagull( void );
	void		AddSeagullPoop( const Vector &vecOrigin );

	void		InputShowHudHint( inputdata_t &inputdata );
	void		InputStartRemoveTauCannon( inputdata_t &inputdata );
	void		InputFinishRemoveTauCannon( inputdata_t &inputdata );

protected:

	virtual void HandleWater( void );
	bool		 CheckWater( void );

	bool			m_bGunHasBeenCutOff;
	float			m_flDangerSoundTime;
	int				m_nBulletType;
	bool			m_bCannonCharging;
	float			m_flCannonTime;
	float			m_flCannonChargeStartTime;
	Vector			m_vecGunOrigin;
	CSoundPatch		*m_sndCannonCharge;
	int				m_nSpinPos;
	float			m_aimYaw;
	float			m_aimPitch;
	float			m_throttleDisableTime;
	float			m_flAmmoCrateCloseTime;

	// handbrake after the fact to keep vehicles from rolling
	float			m_flHandbrakeTime;
	bool			m_bInitialHandbrake;

	float			m_flOverturnedTime;

	Vector			m_vecLastEyePos;
	Vector			m_vecLastEyeTarget;
	Vector			m_vecEyeSpeed;
	Vector			m_vecTargetSpeed;

	JeepWaterData_t	m_WaterData;

	int				m_iNumberOfEntries;
	int				m_nAmmoType;

	// Seagull perching
	float			m_flPlayerExitedTime;	// Time at which the player last left this vehicle
	float			m_flLastSawPlayerAt;	// Time at which we last saw the player
	EHANDLE			m_hLastPlayerInVehicle;
	EHANDLE			m_hSeagull;
	bool			m_bHasPoop;

	CNetworkVar( bool, m_bHeadlightIsOn );
};

#endif // VEHICLE_JEEP_H
