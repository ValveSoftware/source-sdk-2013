//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSICS_CANNISTER_H
#define PHYSICS_CANNISTER_H
#ifdef _WIN32
#pragma once
#endif

#include "player_pickup.h"

class CSteamJet;

class CThrustController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
	{
		angular = m_torqueVector;
		linear = m_thrustVector;
		return SIM_LOCAL_ACCELERATION;
	}

	void CalcThrust( const Vector &position, const Vector &direction, IPhysicsObject *pPhys )
	{
		Vector force = direction * m_thrust * pPhys->GetMass();
		
		// Adjust for the position of the thruster -- apply proper torque)
		pPhys->CalculateVelocityOffset( force, position, &m_thrustVector, &m_torqueVector );
		pPhys->WorldToLocalVector( &m_thrustVector, m_thrustVector );
	}

	Vector			m_thrustVector;
	AngularImpulse	m_torqueVector;
	float			m_thrust;
};

class CPhysicsCannister : public CBaseCombatCharacter, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS( CPhysicsCannister, CBaseCombatCharacter );
public:
	~CPhysicsCannister( void );

	void Spawn( void );
	void Precache( void );
	virtual void OnRestore();
	bool CreateVPhysics();

	DECLARE_DATADESC();
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual QAngle PreferredCarryAngles( void ) { return QAngle( -90, 0, 0 ); }
	virtual bool HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer ) { return true; }

	//
	// Input handlers.
	//
	void InputActivate(inputdata_t &data);
	void InputDeactivate(inputdata_t &data);
	void InputExplode(inputdata_t &data);
	void InputWake( inputdata_t &data );

	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	int ObjectCaps() 
	{ 
		return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE);
	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this );
		}
	}

	void CannisterActivate( CBaseEntity *pActivator, const Vector &thrustOffset );
	void CannisterFire( CBaseEntity *pActivator );
	void Deactivate( void );
	void Explode( CBaseEntity *pAttacker );
	void ExplodeTouch( CBaseEntity *pOther );
	void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	// Don't treat as a live target
	virtual bool IsAlive( void ) { return false; }

	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	void	ShutdownJet( void );
	void	BeginShutdownThink( void );

public:
	virtual bool OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason ) { return true; }
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );
	virtual	CBasePlayer *HasPhysicsAttacker( float dt );
	virtual bool ShouldPuntUseLaunchForces( PhysGunForce_t reason ) 
	{ 
		if ( reason == PHYSGUN_FORCE_LAUNCHED ) 
			return (m_thrustTime!=0);
			
		return false; 
	}
	virtual AngularImpulse PhysGunLaunchAngularImpulse( void ) { return vec3_origin; }
	virtual Vector PhysGunLaunchVelocity( const Vector &forward, float flMass ) { return vec3_origin; }

protected:
	void SetPhysicsAttacker( CBasePlayer *pEntity, float flTime );


public:
	Vector				m_thrustOrigin;
	CThrustController	m_thruster;
	IPhysicsMotionController *m_pController;
	CSteamJet			*m_pJet;
	bool				m_active;
	float				m_thrustTime;
	float				m_damage;
	float				m_damageRadius;

	float				m_activateTime;
	string_t			m_gasSound;

	bool				m_bFired;		// True if this cannister was fire by a weapon

	COutputEvent		m_onActivate;
	COutputEvent		m_OnAwakened;

	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;
	EHANDLE					m_hLauncher;	// Entity that caused this cannister to launch

private:
	Vector CalcLocalThrust( const Vector &offset );
};

#endif // PHYSICS_CANNISTER_H
