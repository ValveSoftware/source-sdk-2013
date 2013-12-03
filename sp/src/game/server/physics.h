//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the abstraction layer for the physics simulation system
// Any calls to the external physics library (ipion) should be made through this
// layer.  Eventually, the physics system will probably become a DLL and made 
// accessible to the client & server side code.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef PHYSICS_H
#define PHYSICS_H

#ifdef _WIN32
#pragma once
#endif

#include "physics_shared.h"

class CBaseEntity;
class IPhysicsMaterial;
class IPhysicsConstraint;
class IPhysicsSpring;
class IPhysicsSurfaceProps;
class CTakeDamageInfo;
class ConVar;

extern IPhysicsMaterial		*g_Material;
extern ConVar phys_pushscale;
extern ConVar phys_timescale;

struct objectparams_t;
extern IPhysicsGameTrace	*physgametrace;

class IPhysicsCollisionSolver;
class IPhysicsCollisionEvent;
class IPhysicsObjectEvent;
extern IPhysicsCollisionSolver * const g_pCollisionSolver;
extern IPhysicsCollisionEvent * const g_pCollisionEventHandler;
extern IPhysicsObjectEvent * const g_pObjectEventHandler;

// HACKHACK: We treat anything >= 500kg as a special "large mass" that does more impact damage
// and has special recovery on crushing/killing other objects
// also causes screen shakes on impact with static/world objects
const float VPHYSICS_LARGE_OBJECT_MASS = 500.0f;

struct gamevcollisionevent_t : public vcollisionevent_t
{
	Vector			preVelocity[2];
	Vector			postVelocity[2];
	AngularImpulse	preAngularVelocity[2];
	CBaseEntity		*pEntities[2];

	void Init( vcollisionevent_t *pEvent ) 
	{ 
		*((vcollisionevent_t *)this) = *pEvent; 
		pEntities[0] = NULL;
		pEntities[1] = NULL;
	}
};

struct triggerevent_t
{
	CBaseEntity		*pTriggerEntity;
	IPhysicsObject	*pTriggerPhysics;
	CBaseEntity		*pEntity;
	IPhysicsObject	*pObject;
	bool			bStart;

	inline void Init( CBaseEntity *triggerEntity, IPhysicsObject *triggerPhysics, CBaseEntity *entity, IPhysicsObject *object, bool startTouch )
	{
		pTriggerEntity = triggerEntity;
		pTriggerPhysics= triggerPhysics;
		pEntity = entity;
		pObject = object;
		bStart = startTouch;
	}
	inline void Clear()
	{
		memset( this, 0, sizeof(*this) );
	}
};

// parse solid parameter overrides out of a string 
void PhysSolidOverride( solid_t &solid, string_t overrideScript );

extern CEntityList *g_pShadowEntities;
#ifdef PORTAL
extern CEntityList *g_pShadowEntities_Main;
#endif
void PhysAddShadow( CBaseEntity *pEntity );
void PhysRemoveShadow( CBaseEntity *pEntity );
bool PhysHasShadow( CBaseEntity *pEntity );

void PhysEnableFloating( IPhysicsObject *pObject, bool bEnable );

void PhysCollisionSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float deltaTime, float speed );
void PhysCollisionScreenShake( gamevcollisionevent_t *pEvent, int index );
void PhysCollisionDust( gamevcollisionevent_t *pEvent, surfacedata_t *phit );
#if HL2_EPISODIC
void PhysCollisionWarpEffect( gamevcollisionevent_t *pEvent, surfacedata_t *phit );
#endif
void PhysBreakSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, Vector vecOrigin );

// plays the impact sound for a particular material
void PhysicsImpactSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed );

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info );
void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex );

// Applies force impulses at a later time
void PhysCallbackImpulse( IPhysicsObject *pPhysicsObject, const Vector &vecCenterForce, const AngularImpulse &vecCenterTorque );

// Sets the velocity at a later time
void PhysCallbackSetVelocity( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity );

// queue up a delete on this object
void PhysCallbackRemove(IServerNetworkable *pRemove);

bool PhysGetDamageInflictorVelocityStartOfFrame( IPhysicsObject *pInflictor, Vector &velocity, AngularImpulse &angVelocity );

// force a physics entity to sleep immediately
void PhysForceEntityToSleep( CBaseEntity *pEntity, IPhysicsObject *pObject );

// teleport an entity to it's position relative to an object it's constrained to
void PhysTeleportConstrainedEntity( CBaseEntity *pTeleportSource, IPhysicsObject *pObject0, IPhysicsObject *pObject1, const Vector &prevPosition, const QAngle &prevAngles, bool physicsRotate );

void PhysGetListOfPenetratingEntities( CBaseEntity *pSearch, CUtlVector<CBaseEntity *> &list );
bool PhysShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1 );

// returns true when processing a callback - so we can defer things that can't be done inside a callback
bool PhysIsInCallback();
bool PhysIsFinalTick();

bool PhysGetTriggerEvent( triggerevent_t *pEvent, CBaseEntity *pTrigger );
// note: pErrorEntity is used to report errors (object not found, more than one found).  It can be NULL
IPhysicsObject *FindPhysicsObjectByName( const char *pName, CBaseEntity *pErrorEntity );
bool PhysFindOrAddVehicleScript( const char *pScriptName, struct vehicleparams_t *pParams, struct vehiclesounds_t *pSounds );
void PhysFlushVehicleScripts();

// this is called to flush all queues when the delete list is cleared
void PhysOnCleanupDeleteList();

struct masscenteroverride_t
{
	enum align_type
	{
		ALIGN_POINT = 0,
		ALIGN_AXIS = 1,
	};

	void Defaults()
	{
		entityName = NULL_STRING;
	}

	void SnapToPoint( string_t name, const Vector &pointWS )
	{
		entityName = name;
		center = pointWS;
		axis.Init();
		alignType = ALIGN_POINT;
	}

	void SnapToAxis( string_t name, const Vector &axisStartWS, const Vector &unitAxisDirWS )
	{
		entityName = name;
		center = axisStartWS;
		axis = unitAxisDirWS;
		alignType = ALIGN_AXIS;
	}

	Vector		center;
	Vector		axis;
	int			alignType;
	string_t	entityName;
};

void PhysSetMassCenterOverride( masscenteroverride_t &override );
// NOTE: this removes the entry from the table as well as retrieving it
void PhysGetMassCenterOverride( CBaseEntity *pEntity, vcollide_t *pCollide, solid_t &solidOut );
float PhysGetEntityMass( CBaseEntity *pEntity );
void PhysSetEntityGameFlags( CBaseEntity *pEntity, unsigned short flags );

void DebugDrawContactPoints(IPhysicsObject *pPhysics);

#endif		// PHYSICS_H
