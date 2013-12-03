//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_PROP_VEHICLE_H
#define C_PROP_VEHICLE_H
#pragma once

#include "iclientvehicle.h"
#include "vehicle_viewblend_shared.h"
class C_PropVehicleDriveable : public C_BaseAnimating, public IClientVehicle
{

	DECLARE_CLASS( C_PropVehicleDriveable, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();
	DECLARE_DATADESC();

	C_PropVehicleDriveable();
	~C_PropVehicleDriveable();

// IVehicle overrides.
public:

	virtual C_BaseCombatCharacter* GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BaseCombatCharacter *pEnt );
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );

	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}

	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}

// IClientVehicle overrides.
public:

	virtual void GetVehicleFOV( float &flFOV ) { flFOV = m_flFOV; }
	virtual void DrawHudElements();
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	virtual void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;

#ifdef HL2_CLIENT_DLL
	virtual int GetPrimaryAmmoType() const { return -1; }
	virtual int GetPrimaryAmmoCount() const { return -1; }
	virtual int GetPrimaryAmmoClip() const  { return -1; }
	virtual bool PrimaryAmmoUsesClips() const { return false; }
#endif

	virtual bool IsPredicted() const { return false; }
	virtual int GetJoystickResponseCurve() const;

// C_BaseEntity overrides.
public:

	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual bool IsSelfAnimating() { return false; };

	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	// Should this object cast render-to-texture shadows?
	virtual ShadowType_t ShadowCastType();

	// Mark the shadow as dirty while the vehicle is being driven
	virtual void ClientThink( void );

// C_PropVehicleDriveable
public:

	bool	IsRunningEnterExitAnim( void ) { return m_bEnterAnimOn || m_bExitAnimOn; }
	// NVNT added to check if the vehicle needs to aim
	virtual bool HasGun(void){return m_bHasGun;}

protected:

	virtual void OnEnteredVehicle( C_BaseCombatCharacter *pPassenger );
	// NVNT added to notify haptics system of vehicle exit.
	virtual void OnExitedVehicle( C_BaseCombatCharacter *pPassenger );

	virtual void RestrictView( float *pYawBounds, float *pPitchBounds, float *pRollBounds, QAngle &vecViewAngles );
	virtual void SetVehicleFOV( float flFOV ) { m_flFOV = flFOV; }

protected:

	CHandle<C_BasePlayer>		m_hPlayer;
	int							m_nSpeed;
	int							m_nRPM;
	float						m_flThrottle;
	int							m_nBoostTimeLeft;
	int							m_nHasBoost;
	int							m_nScannerDisabledWeapons;
	int							m_nScannerDisabledVehicle;

	// timers/flags for flashing icons on hud
	int							m_iFlashTimer;
	bool						m_bLockedDim;
	bool						m_bLockedIcon;

	int							m_iScannerWepFlashTimer;
	bool						m_bScannerWepDim;
	bool						m_bScannerWepIcon;

	int							m_iScannerVehicleFlashTimer;
	bool						m_bScannerVehicleDim;
	bool						m_bScannerVehicleIcon;

	float						m_flSequenceChangeTime;
	bool						m_bEnterAnimOn;
	bool						m_bExitAnimOn;
	float						m_flFOV;

	Vector						m_vecGunCrosshair;
	CInterpolatedVar<Vector>	m_iv_vecGunCrosshair;
	Vector						m_vecEyeExitEndpoint;
	bool						m_bHasGun;
	bool						m_bUnableToFire;

	// Used to smooth view entry
	CHandle<C_BasePlayer>		m_hPrevPlayer;

	ViewSmoothingData_t			m_ViewSmoothingData;
};


#endif // C_PROP_VEHICLE_H
