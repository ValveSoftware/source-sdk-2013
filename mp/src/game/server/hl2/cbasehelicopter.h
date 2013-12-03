//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for helicopters & helicopter-type vehicles
//
// $NoKeywords: $
//=============================================================================//

#ifndef CBASEHELICOPTER_H
#define CBASEHELICOPTER_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_trackpather.h"


//---------------------------------------------------------
//  Helicopter flags
//---------------------------------------------------------
enum HelicopterFlags_t
{
	BITS_HELICOPTER_GUN_ON			= 0x00000001,	// Gun is on and aiming
	BITS_HELICOPTER_MISSILE_ON		= 0x00000002,	// Missile turrets are on and aiming
};


//---------------------------------------------------------
//---------------------------------------------------------
#define SF_NOWRECKAGE		0x08
#define SF_NOROTORWASH		0x20
#define SF_AWAITINPUT		0x40

//---------------------------------------------------------
//---------------------------------------------------------
// Pathing data
#define BASECHOPPER_LEAD_DISTANCE			800.0f
#define	BASECHOPPER_MIN_CHASE_DIST_DIFF		128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it
#define BASECHOPPER_AVOID_DIST				256.0f

#define BASECHOPPER_MAX_SPEED				400.0f
#define BASECHOPPER_MAX_FIRING_SPEED		250.0f
#define BASECHOPPER_MIN_ROCKET_DIST			1000.0f
#define BASECHOPPER_MAX_GUN_DIST			2000.0f

//---------------------------------------------------------
// Physics rotor pushing
#define BASECHOPPER_WASH_RADIUS			256
#define BASECHOPPER_WASH_PUSH_MIN		30		// Initial force * their mass applied to objects in the wash
#define BASECHOPPER_WASH_PUSH_MAX		40		// Maximum force * their mass applied to objects in the wash
#define BASECHOPPER_WASH_RAMP_TIME		1.0		// Time it takes to ramp from the initial to the max force on an object in the wash (at the center of the wash)
#define BASECHOPPER_WASH_MAX_MASS		300		// Don't attempt to push anything over this mass
#define BASECHOPPER_WASH_MAX_OBJECTS	6		// Maximum number of objects the wash will push at once

// Wash physics pushing
struct washentity_t
{
	DECLARE_DATADESC();

	EHANDLE		hEntity;
	float		flWashStartTime;
};

#define BASECHOPPER_WASH_ALTITUDE			1024.0f

//=========================================================
//=========================================================

class CBaseHelicopter : public CAI_TrackPather
{
public:
	DECLARE_CLASS( CBaseHelicopter, CAI_TrackPather );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CBaseHelicopter( void );

	void Spawn( void );
	void Precache( void );
	virtual void UpdateOnRemove();

	void Event_Killed( const CTakeDamageInfo &info );
	void StopLoopingSounds();

	int  BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void );

	Class_T Classify ( void ) { return CLASS_COMBINE; }
			 
	void CallDyingThink( void ) { DyingThink(); }

	bool HasEnemy( void ) { return GetEnemy() != NULL; }
	virtual void GatherEnemyConditions( CBaseEntity *pEnemy );
	virtual bool ChooseEnemy( void );
	virtual void HelicopterPostThink( void ) { };
	virtual void FlyTouch( CBaseEntity *pOther );
	virtual void CrashTouch( CBaseEntity *pOther );
	virtual void HelicopterThink( void );
	virtual void DyingThink( void );
	virtual void NullThink( void );
	virtual void Startup ( void );

	virtual void Flight( void );

	virtual void ShowDamage( void ) {};

	void UpdatePlayerDopplerShift( void );

	virtual void Hunt( void );

	virtual bool IsCrashing( void ) { return m_lifeState != LIFE_ALIVE; }
	virtual float GetAcceleration( void ) { return 5; }

	virtual void ApplySidewaysDrag( const Vector &vecRight );
	virtual void ApplyGeneralDrag( void );

	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	virtual bool FireGun( void );

	virtual float GetRotorVolume( void );
	virtual void InitializeRotorSound( void );
	virtual void UpdateRotorSoundPitch( int iPitch );

	virtual void AimRocketGun(void) {};
	virtual void FireRocket(  Vector vLaunchPos, Vector vLaunchDir  ) {};

	virtual bool	GetTrackPatherTarget( Vector *pPos );
	virtual CBaseEntity *GetTrackPatherTargetEnt();

	void	DrawDebugGeometryOverlays(void);

	// Rotor washes
	virtual void	DrawRotorWash( float flAltitude, const Vector &vecRotorOrigin );
	void			DoRotorPhysicsPush( const Vector &vecRotorOrigin, float flAltitude );
	bool			DoWashPush( washentity_t *pWash, const Vector &vecWashOrigin );
	void			StopRotorWash( void );

	// Purpose: Marks the entity for deletion
	void			InputKill( inputdata_t &inputdata );
	void			DelayedKillThink( );

	virtual			void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );

	// Helicopters never burn
	virtual void	Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner ) { return; }


protected:
	void			HelicopterMove( );

	// Updates the enemy
	void			UpdateEnemy();

	// Override the desired position if your derived helicopter is doing something special
	virtual void	UpdateDesiredPosition( void );

	// Updates the facing direction
	virtual void	UpdateFacingDirection();

	// Fire weapons
	void			FireWeapons();

	// Computes the actual position to fly to
	void			ComputeActualTargetPosition( float flSpeed, float flTime, float flPerpDist, Vector *pDest, bool bApplyNoise = true );

	// Gets the max speed of the helicopter
	virtual float	GetMaxSpeed();
	virtual float	GetMaxSpeedFiring();

	// Updates the enemy
	virtual float	EnemySearchDistance( );

	// Rotor wash think
	void			RotorWashThink( void );

	// Purpose: Push an airboat in our wash
	void			DoWashPushOnAirboat( CBaseEntity *pAirboat, const Vector &vecWashToAirboat, float flWashAmount );

	// Updates the rotor wash volume
	virtual void	UpdateRotorWashVolume();

	// Rotor sound
	void	InputEnableRotorSound( inputdata_t &inputdata );
	void	InputDisableRotorSound( inputdata_t &inputdata );

protected:
	CSoundPatch		*m_pRotorSound;				// Rotor loop played when the player can see the helicopter
	CSoundPatch		*m_pRotorBlast;				// Sound played when the helicopter's pushing around physics objects

	float			m_flForce;
	int				m_fHelicopterFlags;

	Vector			m_vecDesiredFaceDir;

	float			m_flLastSeen;
	float			m_flPrevSeen;

	int				m_iSoundState;		// don't save this

	Vector			m_vecTargetPosition;

	float			m_flMaxSpeed;		// Maximum speed of the helicopter.
	float			m_flMaxSpeedFiring;	// Maximum speed of the helicopter whilst firing guns.

	float			m_flGoalSpeed;		// Goal speed
	float			m_flInitialSpeed;

	float			m_flRandomOffsetTime;
	Vector			m_vecRandomOffset;
	float			m_flRotorWashEntitySearchTime;
	bool			m_bSuppressSound;

	EHANDLE			m_hRotorWash;	// Attached rotorwash entity

	// Inputs
	void			InputActivate( inputdata_t &inputdata );

	// Inputs
	void			InputGunOn( inputdata_t &inputdata );
	void			InputGunOff( inputdata_t &inputdata );
	void			InputMissileOn( inputdata_t &inputdata );
	void			InputMissileOff( inputdata_t &inputdata );
	void			InputEnableRotorWash( inputdata_t &inputdata );
	void			InputDisableRotorWash( inputdata_t &inputdata );
	void			InputMoveTopSpeed( inputdata_t &inputdata );	// Causes the helicopter to immediately accelerate to its desired velocity
	void			InputMoveSpecifiedSpeed( inputdata_t &inputdata );
	void			InputSetAngles( inputdata_t &inputdata );	// Sets the angles of the helicopter

protected:	
	// Custom conservative collision volumes
	Vector			m_cullBoxMins;
	Vector			m_cullBoxMaxs;

	// Wash physics pushing
	CUtlVector< washentity_t >	m_hEntitiesPushedByWash;

	void SetStartupTime( float time ) { m_flStartupTime = time; }
private:
	CNetworkVar( float, m_flStartupTime );
};

//-----------------------------------------------------------------------------
// This entity is used to create little force spheres that the helicopter
// should avoid. 
//-----------------------------------------------------------------------------
class CAvoidSphere : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CAvoidSphere, CBaseEntity );

	void Init( float flRadius );
	virtual void Activate();
	virtual void UpdateOnRemove();

	static void ComputeAvoidanceForces( CBaseEntity *pEntity, float flEntityRadius, float flAvoidTime, Vector *pVecAvoidForce );

private:
	typedef CHandle<CAvoidSphere> AvoidSphereHandle_t;

	float m_flRadius;
	
	static CUtlVector< AvoidSphereHandle_t > s_AvoidSpheres; 
};


//-----------------------------------------------------------------------------
// This entity is used to create little force boxes that the helicopter
// should avoid. 
//-----------------------------------------------------------------------------
class CAvoidBox : public CBaseEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CAvoidBox, CBaseEntity );

	virtual void Spawn( );
	virtual void Activate();
	virtual void UpdateOnRemove();

	static void ComputeAvoidanceForces( CBaseEntity *pEntity, float flEntityRadius, float flAvoidTime, Vector *pVecAvoidForce );

private:
	typedef CHandle<CAvoidBox> AvoidBoxHandle_t;	
	static CUtlVector< AvoidBoxHandle_t > s_AvoidBoxes; 
};


#endif // CBASEHELICOPTER_H
