//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef C_VEHICLE_JEEP_H
#define C_VEHICLE_JEEP_H
#pragma once

#include "cbase.h"
#include "c_prop_vehicle.h"
//#include "movevars_shared.h"
//#include "view.h"
#include "flashlighteffect.h"
//#include "c_baseplayer.h"
//#include "c_te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

//=============================================================================
//
// Client-side Jeep Class
//
class C_PropJeep : public C_PropVehicleDriveable
{

	DECLARE_CLASS( C_PropJeep, C_PropVehicleDriveable );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_PropJeep();
	~C_PropJeep();

public:

	void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );

	void OnEnteredVehicle( C_BasePlayer *pPlayer );
	void Simulate( void );

private:

	void DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );

private:

	Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;
	Vector		m_vecTargetSpeed;

	float		m_flViewAngleDeltaTime;

	float		m_flJeepFOV;
	CHeadlightEffect *m_pHeadlight;
	bool		m_bHeadlightIsOn;
};

#endif // C_VEHICLE_JEEP_H
