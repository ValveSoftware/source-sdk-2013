//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHYSOBJ_H
#define PHYSOBJ_H
#ifdef _WIN32
#pragma once
#endif

#ifndef PHYSICS_H
#include "physics.h"
#endif

#include "entityoutput.h"
#include "func_break.h"
#include "player_pickup.h"

// ---------------------------------------------------------------------
//
// CPhysBox -- physically simulated brush rectangular solid
//
// ---------------------------------------------------------------------
// Physbox Spawnflags. Start at 0x01000 to avoid collision with CBreakable's
#define SF_PHYSBOX_ASLEEP					0x01000
#define SF_PHYSBOX_IGNOREUSE				0x02000
#define SF_PHYSBOX_DEBRIS					0x04000
#define SF_PHYSBOX_MOTIONDISABLED			0x08000
#define SF_PHYSBOX_USEPREFERRED				0x10000
#define SF_PHYSBOX_ENABLE_ON_PHYSCANNON		0x20000
#define SF_PHYSBOX_NO_ROTORWASH_PUSH		0x40000		// The rotorwash doesn't push these
#define SF_PHYSBOX_ENABLE_PICKUP_OUTPUT		0x80000
#define SF_PHYSBOX_ALWAYS_PICK_UP		    0x100000		// Physcannon can always pick this up, no matter what mass or constraints may apply.
#define SF_PHYSBOX_NEVER_PICK_UP			0x200000		// Physcannon will never be able to pick this up.
#define SF_PHYSBOX_NEVER_PUNT				0x400000		// Physcannon will never be able to punt this object.
#define SF_PHYSBOX_PREVENT_PLAYER_TOUCH_ENABLE 0x800000		// If set, the player will not cause the object to enable its motion when bumped into

// UNDONE: Hook collisions into the physics system to generate touch functions and take damage on falls
// UNDONE: Base class PhysBrush
class CPhysBox : public CBreakable
{
DECLARE_CLASS( CPhysBox, CBreakable );

public:
	DECLARE_SERVERCLASS();

	void	Spawn ( void );
	bool	CreateVPhysics();
	void	Move( const Vector &force );
	virtual int ObjectCaps();
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	virtual int DrawDebugTextOverlays(void);

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	int		OnTakeDamage( const CTakeDamageInfo &info );
	void		 EnableMotion( void );

	bool CanBePickedUpByPhyscannon();

	// IPlayerPickupVPhysics
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );

	bool		 HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer );
	virtual QAngle PreferredCarryAngles( void ) { return m_angPreferredCarryAngles; }

	// inputs
	void InputWake( inputdata_t &inputdata );
	void InputSleep( inputdata_t &inputdata );
	void InputEnableMotion( inputdata_t &inputdata );
	void InputDisableMotion( inputdata_t &inputdata );
	void InputForceDrop( inputdata_t &inputdata );
	void InputDisableFloating( inputdata_t &inputdata );

	DECLARE_DATADESC();
	
protected:
	int				m_damageType;
	float			m_massScale;
	string_t		m_iszOverrideScript;
	int				m_damageToEnableMotion;
	float			m_flForceToEnableMotion;
	QAngle			m_angPreferredCarryAngles;
	bool			m_bNotSolidToWorld;

	// Outputs
	COutputEvent	m_OnDamaged;
	COutputEvent	m_OnAwakened;
	COutputEvent	m_OnMotionEnabled;
	COutputEvent	m_OnPhysGunPickup;
	COutputEvent	m_OnPhysGunPunt;
	COutputEvent	m_OnPhysGunOnlyPickup;
	COutputEvent	m_OnPhysGunDrop;
	COutputEvent	m_OnPlayerUse;

	CHandle<CBasePlayer>	m_hCarryingPlayer;	// Player who's carrying us
};

// ---------------------------------------------------------------------
//
// CPhysExplosion -- physically simulated explosion
//
// ---------------------------------------------------------------------
class CPhysExplosion : public CPointEntity
{
public:
	DECLARE_CLASS( CPhysExplosion, CPointEntity );

	void	Spawn ( void );
	void	Explode( CBaseEntity *pActivator, CBaseEntity *pCaller );

	CBaseEntity *FindEntity( CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller );

	int DrawDebugTextOverlays(void);

	// Input handlers
	void InputExplode( inputdata_t &inputdata );

	DECLARE_DATADESC();
private:
	
	float		GetRadius( void );

	float		m_damage;
	float		m_radius;
	string_t	m_targetEntityName;
	float		m_flInnerRadius;
	
	COutputEvent	m_OnPushedPlayer;	
};


//==================================================
// CPhysImpact
//==================================================

class CPhysImpact : public CPointEntity
{
public:
	DECLARE_CLASS( CPhysImpact, CPointEntity );

	void		Spawn( void );
	//void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void		Activate( void );

	void		InputImpact( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:

	void		PointAtEntity( void );

	float		m_damage;
	float		m_distance;
	string_t	m_directionEntityName;
};

//-----------------------------------------------------------------------------
// Purpose: A magnet that creates constraints between itself and anything it touches 
//-----------------------------------------------------------------------------

struct magnetted_objects_t
{
	IPhysicsConstraint *pConstraint;
	EHANDLE			   hEntity;

	DECLARE_SIMPLE_DATADESC();
};

class CPhysMagnet : public CBaseAnimating, public IPhysicsConstraintEvent
{
	DECLARE_CLASS( CPhysMagnet, CBaseAnimating );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CPhysMagnet();
	~CPhysMagnet();

	void	Spawn( void );
	void	Precache( void );
	void	Touch( CBaseEntity *pOther );
	void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void	DoMagnetSuck( CBaseEntity *pOther );
	void	SetConstraintGroup( IPhysicsConstraintGroup *pGroup );

	bool	IsOn( void ) { return m_bActive; }
	int		GetNumAttachedObjects( void );
	float	GetTotalMassAttachedObjects( void );
	CBaseEntity *GetAttachedObject( int iIndex );

	// Checking for hitting something
	void	ResetHasHitSomething( void ) { m_bHasHitSomething = false; }
	bool	HasHitSomething( void ) { return m_bHasHitSomething; }

	// Inputs
	void	InputToggle( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	void	InputConstraintBroken( inputdata_t &inputdata );

	void	DetachAll( void );

// IPhysicsConstraintEvent
public:
	void	ConstraintBroken( IPhysicsConstraint *pConstraint );
	
protected:
	// Outputs
	COutputEvent	m_OnMagnetAttach;
	COutputEvent	m_OnMagnetDetach;

	// Keys
	float			m_massScale;
	string_t		m_iszOverrideScript;
	float			m_forceLimit;
	float			m_torqueLimit;

	CUtlVector< magnetted_objects_t >	m_MagnettedEntities;
	IPhysicsConstraintGroup				*m_pConstraintGroup;

	bool			m_bActive;
	bool			m_bHasHitSomething;
	float			m_flTotalMass;
	float			m_flRadius;
	float			m_flNextSuckTime;
	int				m_iMaxObjectsAttached;
};

#endif // PHYSOBJ_H
